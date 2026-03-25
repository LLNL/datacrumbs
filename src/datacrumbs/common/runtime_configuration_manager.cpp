#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/probe_file.h>
#include <datacrumbs/common/runtime_configuration_manager.h>
#include <datacrumbs/common/singleton.h>
#include <json-c/json.h>
#include <pwd.h>
#include <sqlite3.h>
#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unordered_map>

namespace datacrumbs {

namespace {

constexpr uint64_t kRuntimeProbeEventIdBase = 1000;
constexpr int kSqliteBusyTimeoutMs = 5000;

struct InvalidProbeBucket {
  std::string group;
  std::string scope_key;
};

std::string json_string_or_empty(json_object* root, const char* key) {
  json_object* value = nullptr;
  if (!root || !json_object_object_get_ex(root, key, &value) ||
      json_object_get_type(value) != json_type_string) {
    return "";
  }
  return json_object_get_string(value);
}

std::unordered_map<std::string, std::string> load_sqlite_kv_table(
    sqlite3* db, const char* table_name, const std::filesystem::path& database_path) {
  std::unordered_map<std::string, std::string> values;
  const std::string sql = std::string("SELECT key, value FROM ") + table_name + ";";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
    DC_LOG_WARN("[RuntimeConfigurationManager] Failed to query %s from %s: %s", table_name,
                database_path.string().c_str(), sqlite3_errmsg(db));
    return values;
  }

  while (sqlite3_step(statement) == SQLITE_ROW) {
    const unsigned char* key = sqlite3_column_text(statement, 0);
    const unsigned char* value = sqlite3_column_text(statement, 1);
    if (!key || !value) {
      continue;
    }
    values[reinterpret_cast<const char*>(key)] = reinterpret_cast<const char*>(value);
  }

  sqlite3_finalize(statement);
  return values;
}

std::shared_ptr<Probe> probe_from_json(json_object* probe_obj) {
  if (!probe_obj) return nullptr;
  Probe base = Probe::fromJson(probe_obj);
  switch (base.type) {
    case ProbeType::SYSCALLS:
      return std::make_shared<SysCallProbe>(SysCallProbe::fromJson(probe_obj));
    case ProbeType::KPROBE:
      return std::make_shared<KProbe>(KProbe::fromJson(probe_obj));
    case ProbeType::UPROBE:
      return std::make_shared<UProbe>(UProbe::fromJson(probe_obj));
    case ProbeType::USDT:
      return std::make_shared<USDTProbe>(USDTProbe::fromJson(probe_obj));
    case ProbeType::CUSTOM:
      return std::make_shared<CustomProbe>(CustomProbe::fromJson(probe_obj));
    default:
      return nullptr;
  }
}

std::string runtime_event_key(const std::string& probe_name, const std::string& function_name) {
  return probe_name + "\n" + function_name;
}

bool is_supported_runtime_probe_type(ProbeType type) {
  switch (type) {
    case ProbeType::KPROBE:
    case ProbeType::UPROBE:
    case ProbeType::SYSCALLS:
    case ProbeType::USDT:
      return true;
    default:
      return false;
  }
}

InvalidProbeBucket invalid_probe_bucket(const std::shared_ptr<Probe>& probe) {
  if (!probe) {
    return {"unknown", ""};
  }

  switch (probe->type) {
    case ProbeType::SYSCALLS:
      return {"syscalls", ""};
    case ProbeType::KPROBE:
      return {"kprobes", ""};
    case ProbeType::UPROBE: {
      const auto uprobe = std::dynamic_pointer_cast<UProbe>(probe);
      return {"binary", uprobe ? uprobe->binary_path : ""};
    }
    case ProbeType::USDT: {
      const auto usdt = std::dynamic_pointer_cast<USDTProbe>(probe);
      if (!usdt) {
        return {"usdt", ""};
      }
      return {"usdt", usdt->binary_path + "\n" + usdt->provider};
    }
    default:
      return {"unknown", ""};
  }
}

bool sqlite_exec(sqlite3* db, const char* sql, const std::filesystem::path& path,
                 const char* operation) {
  char* error_message = nullptr;
  const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error_message);
  if (rc == SQLITE_OK) {
    return true;
  }

  const char* sqlite_message = error_message ? error_message : sqlite3_errmsg(db);
  DC_LOG_WARN("[RuntimeConfigurationManager] Failed to %s %s: %s", operation, path.string().c_str(),
              sqlite_message ? sqlite_message : "unknown sqlite error");
  if (error_message) {
    sqlite3_free(error_message);
  }
  return false;
}

sqlite3* open_runtime_probe_state_database(const std::filesystem::path& database_path, int flags) {
  sqlite3* db = nullptr;
  if (sqlite3_open_v2(database_path.c_str(), &db, flags, nullptr) != SQLITE_OK) {
    DC_LOG_WARN("[RuntimeConfigurationManager] Failed to open runtime probe state database %s: %s",
                database_path.string().c_str(), db ? sqlite3_errmsg(db) : "sqlite open failed");
    if (db) {
      sqlite3_close(db);
    }
    return nullptr;
  }

  sqlite3_busy_timeout(db, kSqliteBusyTimeoutMs);
  return db;
}

bool initialize_runtime_probe_state_database(sqlite3* db,
                                             const std::filesystem::path& database_path) {
  if (!sqlite_exec(db, "PRAGMA journal_mode=WAL;", database_path, "enable WAL mode")) {
    return false;
  }
  if (!sqlite_exec(db, "PRAGMA synchronous=NORMAL;", database_path, "set synchronous mode")) {
    return false;
  }
  return sqlite_exec(db,
                     "CREATE TABLE IF NOT EXISTS runtime_probe_status ("
                     "probe_group TEXT NOT NULL,"
                     "scope_key TEXT NOT NULL,"
                     "function_name TEXT NOT NULL,"
                     "status TEXT NOT NULL CHECK (status IN ('invalid', 'successful')),"
                     "PRIMARY KEY (probe_group, scope_key, function_name, status)"
                     ");",
                     database_path, "create runtime probe state table");
}

void ensure_directory_owned_by_install_user(const std::filesystem::path& directory) {
  if (directory.empty()) {
    return;
  }

  struct passwd* pwd = getpwnam(DATACRUMBS_INSTALL_USER);

  std::error_code ec;
  std::filesystem::path current;
  for (const auto& part : directory) {
    current /= part;
    if (current.empty()) {
      continue;
    }
    std::filesystem::create_directory(current, ec);
    ec.clear();
  }

  if (pwd == nullptr) {
    return;
  }

  chown(directory.c_str(), pwd->pw_uid, pwd->pw_gid);
  chmod(directory.c_str(), S_IRWXU | S_IRWXG);
}

void replace_all(std::string* target, const std::string& from, const std::string& to) {
  if (from.empty()) return;
  size_t pos = 0;
  while ((pos = target->find(from, pos)) != std::string::npos) {
    target->replace(pos, from.size(), to);
    pos += to.size();
  }
}

std::string expand_trace_dir_pattern(const std::string& pattern) {
  if (pattern.empty()) return "";

  std::time_t now = std::time(nullptr);
  std::tm tm_now{};
  localtime_r(&now, &tm_now);
  char yy[3];
  char mm[3];
  char dd[3];
  std::snprintf(yy, sizeof(yy), "%02d", (tm_now.tm_year + 1900) % 100);
  std::snprintf(mm, sizeof(mm), "%02d", tm_now.tm_mon + 1);
  std::snprintf(dd, sizeof(dd), "%02d", tm_now.tm_mday);

  std::string expanded = pattern;
  replace_all(&expanded, "%YY%", yy);
  replace_all(&expanded, "%MM%", mm);
  replace_all(&expanded, "%DD%", dd);
  return expanded;
}

void persist_run_id(const std::filesystem::path& run_id_file, const std::string& run_id) {
  if (run_id_file.empty()) return;
  std::error_code ec;
  std::filesystem::create_directories(run_id_file.parent_path(), ec);
  ec.clear();

  std::ofstream output(run_id_file);
  if (!output.is_open()) {
    return;
  }
  output << run_id;
  output.close();
  if (!output) return;
}

void remove_file_if_exists(const std::filesystem::path& path) {
  if (path.empty()) return;
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

void raise_limit_to_hard(int resource, const char* resource_name) {
  struct rlimit limits{};
  if (getrlimit(resource, &limits) != 0) {
    return;
  }
  if (limits.rlim_cur == limits.rlim_max) {
    return;
  }
  limits.rlim_cur = limits.rlim_max;
  if (setrlimit(resource, &limits) == 0) {
    DC_LOG_INFO("Raised %s soft limit to hard limit", resource_name);
  }
}

}  // namespace

template <>
std::shared_ptr<datacrumbs::RuntimeConfigurationManager>
    datacrumbs::Singleton<datacrumbs::RuntimeConfigurationManager>::instance = nullptr;

template <>
bool datacrumbs::Singleton<datacrumbs::RuntimeConfigurationManager>::stop_creating_instances =
    false;

RuntimeConfigurationManager::RuntimeConfigurationManager() {
  throw std::runtime_error(
      "RuntimeConfigurationManager must be initialized with a probes json path.");
}

RuntimeConfigurationManager::RuntimeConfigurationManager(
    const std::filesystem::path& runtime_probe_file, const std::string& explicit_run_id,
    const std::string& explicit_user, bool print)
    : data_dir(DATACRUMBS_INSTALL_SHARED_DATA_DIR),
      trace_log_dir(),
      probe_file_path(runtime_probe_file),
      system_probe_path(DATACRUMBS_SYSTEM_PROBE_FILE),
      server_run_dir(DATACRUMBS_SERVER_RUN_DIR),
      server_run_id_file(DATACRUMBS_SERVER_RUN_ID_FILE),
      user(explicit_user),
      log_dir(DATACRUMBS_LOG_DIR),
      run_id(explicit_run_id),
      trace_dir_pattern(DATACRUMBS_TRACE_DIR_PATTERN),
      inclusion_paths("") {
  load_runtime_system_configuration();
  raise_limit_to_hard(RLIMIT_NOFILE, "RLIMIT_NOFILE");
  raise_limit_to_hard(RLIMIT_AS, "RLIMIT_AS");
  raise_limit_to_hard(RLIMIT_MEMLOCK, "RLIMIT_MEMLOCK");
  derive_configurations();
  remove_file_if_exists(server_ready_file);
  persist_run_id(server_run_id_file, run_id);
  probe_file_path = runtime_probe_file;
  load_runtime_probe_file();
  validate_configurations();
  if (print) {
    print_configurations();
  }
}

void RuntimeConfigurationManager::print_configurations() const {
  DC_LOG_INFO("[RuntimeConfigurationManager] Final configuration:");
  DC_LOG_INFO("  Trace log dir: %s", trace_log_dir.string().c_str());
  DC_LOG_INFO("  Trace file path: %s", trace_file_path.string().c_str());
  DC_LOG_INFO("  Trace dir pattern: %s",
              trace_dir_pattern.empty() ? "<none>" : trace_dir_pattern.c_str());
  DC_LOG_INFO("  Data dir: %s", data_dir.string().c_str());
  DC_LOG_INFO("  Probe file path: %s", probe_file_path.string().c_str());
  DC_LOG_INFO("  System probe path: %s", system_probe_path.string().c_str());
  DC_LOG_INFO("  Runtime probe state database: %s", runtime_probe_state_db_path.string().c_str());
  DC_LOG_INFO("  Run dir: %s", server_run_dir.string().c_str());
  DC_LOG_INFO("  Run ID file: %s", server_run_id_file.string().c_str());
  DC_LOG_INFO("  Ready file: %s", server_ready_file.string().c_str());
  DC_LOG_INFO("  Runtime user: %s", user.c_str());
  DC_LOG_INFO("  Install user: %s", DATACRUMBS_INSTALL_USER);
  DC_LOG_INFO("  Hostname: %s", hostname.c_str());
  DC_LOG_INFO("  Inclusion paths: %s",
              inclusion_paths.empty() ? "<none>" : inclusion_paths.c_str());
  DC_LOG_INFO("  Runtime probes loaded: %zu", runtime_probes.size());
  DC_LOG_INFO("  Category map entries: %zu", category_map.size());
}

void RuntimeConfigurationManager::derive_configurations() {
  char hostname_buf[256] = {0};
  if (gethostname(hostname_buf, sizeof(hostname_buf) - 1) != 0) {
    throw std::runtime_error("Failed to get hostname.");
  }
  hostname = hostname_buf;

  if (!trace_dir_pattern.empty()) {
    trace_log_dir = expand_trace_dir_pattern(trace_dir_pattern);
  }
  ensure_directory_owned_by_install_user(trace_log_dir);

  const std::string probe_stem = probe_file_path.stem().string();
  const std::string generated_file_suffix = user + "-" + run_id + "-" + hostname + "-" + probe_stem;
  const std::string lookup_file_suffix = std::string(DATACRUMBS_INSTALL_USER) + "-" + hostname;
  trace_file_path = trace_log_dir / ("trace-" + generated_file_suffix + ".pfw.gz");
  runtime_probe_state_db_path =
      data_dir / ("probes-runtime-status-" + lookup_file_suffix + ".sqlite");
  server_ready_file = server_run_dir / ("datacrumbs-" + run_id + ".ready");
}

void RuntimeConfigurationManager::load_runtime_system_configuration() {
  sqlite3* db = open_runtime_probe_state_database(system_probe_path, SQLITE_OPEN_READONLY);
  if (!db) {
    throw std::runtime_error("Failed to open system configuration database: " +
                             system_probe_path.string());
  }

  const auto summary = load_sqlite_kv_table(db, "summary", system_probe_path);
  const auto system_configuration =
      load_sqlite_kv_table(db, "system_configuration", system_probe_path);
  sqlite3_close(db);

  if (const auto it = summary.find("trace_log_dir");
      it != summary.end() && trace_dir_pattern.empty() && !it->second.empty()) {
    trace_log_dir = it->second;
  }

  if (const auto it = system_configuration.find("DATACRUMBS_LOG_DIR");
      it != system_configuration.end() && !it->second.empty()) {
    log_dir = it->second;
  }
  if (const auto it = system_configuration.find("DATACRUMBS_INSTALL_DATA_DIR");
      it != system_configuration.end() && !it->second.empty()) {
    data_dir = it->second;
  }
  if (const auto it = system_configuration.find("DATACRUMBS_INCLUSION_PATHS");
      it != system_configuration.end() && inclusion_paths.empty() && !it->second.empty()) {
    inclusion_paths = it->second;
  }
  if (const auto it = system_configuration.find("DATACRUMBS_TRACE_DIR_PATTERN");
      it != system_configuration.end() && !it->second.empty()) {
    trace_dir_pattern = it->second;
  }
  if (const auto it = system_configuration.find("DATACRUMBS_SERVER_RUN_DIR");
      it != system_configuration.end() && !it->second.empty()) {
    server_run_dir = it->second;
  }
  if (const auto it = system_configuration.find("DATACRUMBS_SERVER_RUN_ID_FILE");
      it != system_configuration.end() && !it->second.empty()) {
    server_run_id_file = it->second;
  }
}

void RuntimeConfigurationManager::load_runtime_probe_file() {
  std::string probe_error;
  json_object* categories =
      datacrumbs::probe_file::load_verified_categories_from_file(probe_file_path, &probe_error);
  if (!categories || json_object_get_type(categories) != json_type_array) {
    if (categories) json_object_put(categories);
    throw std::runtime_error("Failed to verify runtime probe file: " + probe_file_path.string() +
                             " (" + probe_error + ")");
  }

  runtime_probes.clear();
  category_map.clear();
  runtime_event_ids.clear();
  runtime_event_metadata.clear();
  invalid_runtime_probes.clear();
  successful_runtime_probes.clear();
  load_runtime_probe_state();

  size_t total_runtime_functions = 0;
  size_t skipped_known_invalid_functions = 0;

  uint64_t event_id = kRuntimeProbeEventIdBase;
  const int arr_len = json_object_array_length(categories);
  for (int i = 0; i < arr_len; ++i) {
    json_object* probe_obj = json_object_array_get_idx(categories, i);
    auto probe = probe_from_json(probe_obj);
    if (!probe) {
      DC_LOG_WARN("[RuntimeConfigurationManager] Skipping unsupported runtime probe at index %d",
                  i);
      continue;
    }
    if (!is_supported_runtime_probe_type(probe->type)) {
      DC_LOG_WARN(
          "[RuntimeConfigurationManager] Skipping non-attachable runtime probe type %d for %s",
          static_cast<int>(probe->type), probe->name.c_str());
      continue;
    }

    std::vector<std::string> filtered_functions;
    filtered_functions.reserve(probe->functions.size());
    for (const auto& function_name : probe->functions) {
      if (is_known_invalid_runtime_probe(probe, function_name)) {
        ++skipped_known_invalid_functions;
        continue;
      }
      filtered_functions.push_back(function_name);
    }
    probe->functions = std::move(filtered_functions);
    runtime_probes.push_back(probe);
    total_runtime_functions += probe->functions.size();
    for (const auto& function_name : probe->functions) {
      const uint64_t assigned_event_id = event_id++;
      category_map[assigned_event_id] = std::make_pair(probe->name, function_name);
      runtime_event_ids[runtime_event_key(probe->name, function_name)] = assigned_event_id;
      RuntimeEventMetadata metadata;
      metadata.probe_type = probe->type;
      metadata.probe_name = probe->name;
      metadata.function_name = function_name;
      if (const auto* arg_specs = probe->getArgSpecs(function_name); arg_specs != nullptr) {
        metadata.arg_specs = *arg_specs;
      }
      runtime_event_metadata[assigned_event_id] = std::move(metadata);
    }
  }

  json_object_put(categories);

  if (skipped_known_invalid_functions > 0) {
    DC_LOG_WARN(
        "[RuntimeConfigurationManager] Skipped %zu known invalid runtime probe functions "
        "using %s",
        skipped_known_invalid_functions, runtime_probe_state_db_path.string().c_str());
  }

  if (total_runtime_functions > DATACRUMBS_MAX_RUNTIME_FUNCTIONS) {
    throw std::runtime_error("Runtime probe file selects " +
                             std::to_string(total_runtime_functions) +
                             " functions, but datacrumbs supports at most " +
                             std::to_string(DATACRUMBS_MAX_RUNTIME_FUNCTIONS) +
                             " functions per run. Regenerate the probes file with fewer "
                             "selected functions.");
  }
}

void RuntimeConfigurationManager::load_runtime_probe_state() {
  if (runtime_probe_state_db_path.empty() ||
      !std::filesystem::exists(runtime_probe_state_db_path)) {
    return;
  }

  sqlite3* db =
      open_runtime_probe_state_database(runtime_probe_state_db_path, SQLITE_OPEN_READONLY);
  if (!db) {
    return;
  }

  sqlite3_stmt* statement = nullptr;
  const char* sql =
      "SELECT probe_group, scope_key, function_name, status FROM runtime_probe_status;";
  if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
    DC_LOG_WARN("[RuntimeConfigurationManager] Failed to query runtime probe state database %s: %s",
                runtime_probe_state_db_path.string().c_str(), sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  while (sqlite3_step(statement) == SQLITE_ROW) {
    const unsigned char* probe_group = sqlite3_column_text(statement, 0);
    const unsigned char* scope_key = sqlite3_column_text(statement, 1);
    const unsigned char* function_name = sqlite3_column_text(statement, 2);
    const unsigned char* status = sqlite3_column_text(statement, 3);
    if (!probe_group || !scope_key || !function_name || !status) {
      continue;
    }
    auto& target = std::string(reinterpret_cast<const char*>(status)) == "successful"
                       ? successful_runtime_probes
                       : invalid_runtime_probes;
    target[reinterpret_cast<const char*>(probe_group)][reinterpret_cast<const char*>(scope_key)]
        .insert(reinterpret_cast<const char*>(function_name));
  }

  sqlite3_finalize(statement);
  sqlite3_close(db);
}

bool RuntimeConfigurationManager::is_known_invalid_runtime_probe(
    const std::shared_ptr<Probe>& probe, const std::string& function_name) const {
  const auto bucket = invalid_probe_bucket(probe);
  const auto group_it = invalid_runtime_probes.find(bucket.group);
  if (group_it == invalid_runtime_probes.end()) {
    return false;
  }
  const auto scope_it = group_it->second.find(bucket.scope_key);
  if (scope_it == group_it->second.end()) {
    return false;
  }
  const auto successful_group_it = successful_runtime_probes.find(bucket.group);
  if (successful_group_it != successful_runtime_probes.end()) {
    const auto successful_scope_it = successful_group_it->second.find(bucket.scope_key);
    if (successful_scope_it != successful_group_it->second.end() &&
        successful_scope_it->second.find(function_name) != successful_scope_it->second.end()) {
      return false;
    }
  }
  return scope_it->second.find(function_name) != scope_it->second.end();
}

void RuntimeConfigurationManager::record_invalid_runtime_probe(const std::shared_ptr<Probe>& probe,
                                                               const std::string& function_name) {
  const auto bucket = invalid_probe_bucket(probe);
  invalid_runtime_probes[bucket.group][bucket.scope_key].insert(function_name);
}

void RuntimeConfigurationManager::record_successful_runtime_probe(
    const std::shared_ptr<Probe>& probe, const std::string& function_name) {
  const auto bucket = invalid_probe_bucket(probe);
  successful_runtime_probes[bucket.group][bucket.scope_key].insert(function_name);

  const auto invalid_group_it = invalid_runtime_probes.find(bucket.group);
  if (invalid_group_it == invalid_runtime_probes.end()) {
    return;
  }
  const auto invalid_scope_it = invalid_group_it->second.find(bucket.scope_key);
  if (invalid_scope_it == invalid_group_it->second.end()) {
    return;
  }

  invalid_scope_it->second.erase(function_name);
  if (invalid_scope_it->second.empty()) {
    invalid_group_it->second.erase(invalid_scope_it);
  }
  if (invalid_group_it->second.empty()) {
    invalid_runtime_probes.erase(invalid_group_it);
  }
}

void RuntimeConfigurationManager::persist_runtime_probe_state() const {
  if (runtime_probe_state_db_path.empty()) {
    return;
  }

  std::error_code ec;
  std::filesystem::create_directories(runtime_probe_state_db_path.parent_path(), ec);

  sqlite3* db = open_runtime_probe_state_database(runtime_probe_state_db_path,
                                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
  if (!db) {
    return;
  }

  if (!initialize_runtime_probe_state_database(db, runtime_probe_state_db_path)) {
    sqlite3_close(db);
    return;
  }
  if (!sqlite_exec(db, "BEGIN IMMEDIATE TRANSACTION;", runtime_probe_state_db_path,
                   "begin transaction")) {
    sqlite3_close(db);
    return;
  }

  sqlite3_stmt* insert_statement = nullptr;
  const char* insert_sql =
      "INSERT OR IGNORE INTO runtime_probe_status "
      "(probe_group, scope_key, function_name, status) VALUES (?, ?, ?, ?);";
  if (sqlite3_prepare_v2(db, insert_sql, -1, &insert_statement, nullptr) != SQLITE_OK) {
    DC_LOG_WARN(
        "[RuntimeConfigurationManager] Failed to prepare runtime probe state insert for %s: %s",
        runtime_probe_state_db_path.string().c_str(), sqlite3_errmsg(db));
    sqlite_exec(db, "ROLLBACK;", runtime_probe_state_db_path, "rollback transaction");
    sqlite3_close(db);
    return;
  }

  sqlite3_stmt* delete_invalid_statement = nullptr;
  const char* delete_invalid_sql =
      "DELETE FROM runtime_probe_status WHERE probe_group = ? AND scope_key = ? "
      "AND function_name = ? AND status = 'invalid';";
  if (sqlite3_prepare_v2(db, delete_invalid_sql, -1, &delete_invalid_statement, nullptr) !=
      SQLITE_OK) {
    DC_LOG_WARN(
        "[RuntimeConfigurationManager] Failed to prepare runtime probe state cleanup for %s: %s",
        runtime_probe_state_db_path.string().c_str(), sqlite3_errmsg(db));
    sqlite3_finalize(insert_statement);
    sqlite_exec(db, "ROLLBACK;", runtime_probe_state_db_path, "rollback transaction");
    sqlite3_close(db);
    return;
  }

  const auto insert_entries = [&](const auto& probe_sets, const char* status) {
    for (const auto& [probe_group, scopes] : probe_sets) {
      for (const auto& [scope_key, functions] : scopes) {
        for (const auto& function_name : functions) {
          sqlite3_reset(insert_statement);
          sqlite3_clear_bindings(insert_statement);
          sqlite3_bind_text(insert_statement, 1, probe_group.c_str(), -1, SQLITE_TRANSIENT);
          sqlite3_bind_text(insert_statement, 2, scope_key.c_str(), -1, SQLITE_TRANSIENT);
          sqlite3_bind_text(insert_statement, 3, function_name.c_str(), -1, SQLITE_TRANSIENT);
          sqlite3_bind_text(insert_statement, 4, status, -1, SQLITE_STATIC);
          if (sqlite3_step(insert_statement) != SQLITE_DONE) {
            DC_LOG_WARN(
                "[RuntimeConfigurationManager] Failed to persist %s runtime probe entry to %s: %s",
                status, runtime_probe_state_db_path.string().c_str(), sqlite3_errmsg(db));
          }
        }
      }
    }
  };

  for (const auto& [probe_group, scopes] : successful_runtime_probes) {
    for (const auto& [scope_key, functions] : scopes) {
      for (const auto& function_name : functions) {
        sqlite3_reset(delete_invalid_statement);
        sqlite3_clear_bindings(delete_invalid_statement);
        sqlite3_bind_text(delete_invalid_statement, 1, probe_group.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(delete_invalid_statement, 2, scope_key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(delete_invalid_statement, 3, function_name.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(delete_invalid_statement) != SQLITE_DONE) {
          DC_LOG_WARN(
              "[RuntimeConfigurationManager] Failed to clear stale invalid runtime probe entry "
              "from %s: %s",
              runtime_probe_state_db_path.string().c_str(), sqlite3_errmsg(db));
        }
      }
    }
  }

  insert_entries(successful_runtime_probes, "successful");
  insert_entries(invalid_runtime_probes, "invalid");

  sqlite3_finalize(delete_invalid_statement);
  sqlite3_finalize(insert_statement);
  if (!sqlite_exec(db, "COMMIT;", runtime_probe_state_db_path, "commit transaction")) {
    sqlite3_close(db);
    return;
  }
  sqlite3_close(db);

  if (struct passwd* pwd = getpwnam(DATACRUMBS_INSTALL_USER); pwd != nullptr) {
    chown(runtime_probe_state_db_path.c_str(), pwd->pw_uid, pwd->pw_gid);
  }
  chmod(runtime_probe_state_db_path.c_str(), S_IRUSR | S_IWUSR);
}

std::optional<uint64_t> RuntimeConfigurationManager::get_runtime_event_id(
    const std::string& probe_name, const std::string& function_name) const {
  const auto it = runtime_event_ids.find(runtime_event_key(probe_name, function_name));
  if (it == runtime_event_ids.end()) {
    return std::nullopt;
  }
  return it->second;
}

const RuntimeEventMetadata* RuntimeConfigurationManager::get_runtime_event_metadata(
    uint64_t event_id) const {
  const auto it = runtime_event_metadata.find(event_id);
  if (it == runtime_event_metadata.end()) {
    return nullptr;
  }
  return &it->second;
}

void RuntimeConfigurationManager::validate_configurations() const {
  if (probe_file_path.empty()) {
    throw std::runtime_error("Runtime probe file path is empty.");
  }
  if (run_id.empty()) {
    throw std::runtime_error("Runtime run id is empty.");
  }
  if (user.empty()) {
    throw std::runtime_error("Runtime user is empty.");
  }
  if (!std::filesystem::exists(probe_file_path)) {
    throw std::runtime_error("Runtime probe file does not exist: " + probe_file_path.string());
  }
  if (trace_log_dir.empty() || !std::filesystem::exists(trace_log_dir)) {
    throw std::runtime_error("Trace log directory does not exist: " + trace_log_dir.string());
  }
  if (system_probe_path.empty() || !std::filesystem::exists(system_probe_path)) {
    throw std::runtime_error("System probe file does not exist: " + system_probe_path.string());
  }
  if (server_run_id_file.empty()) {
    throw std::runtime_error("Run-id file path is empty.");
  }
  if (server_run_dir.empty()) {
    throw std::runtime_error("Run directory path is empty.");
  }
}

}  // namespace datacrumbs
