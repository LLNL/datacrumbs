#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/probe_file.h>
#include <datacrumbs/common/runtime_configuration_manager.h>
#include <datacrumbs/common/singleton.h>
#include <json-c/json.h>
#include <pwd.h>
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

namespace datacrumbs {

namespace {

constexpr uint64_t kRuntimeProbeEventIdBase = 1000;

std::string json_string_or_empty(json_object* root, const char* key) {
  json_object* value = nullptr;
  if (!root || !json_object_object_get_ex(root, key, &value) ||
      json_object_get_type(value) != json_type_string) {
    return "";
  }
  return json_object_get_string(value);
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
    : config_path(DATACRUMBS_CONFIG_PATH),
      data_dir(DATACRUMBS_INSTALL_SHARED_DATA_DIR),
      trace_log_dir(),
      probe_file_path(runtime_probe_file),
      system_probe_path(DATACRUMBS_SYSTEM_PROBE_FILE),
      server_run_dir(DATACRUMBS_SERVER_RUN_DIR),
      server_run_id_file(DATACRUMBS_SERVER_RUN_ID_FILE),
      config_name(runtime_probe_file.stem().string()),
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
  DC_LOG_INFO("  Config path: %s", config_path.string().c_str());
  DC_LOG_INFO("  Config name: %s", config_name.c_str());
  DC_LOG_INFO("  Trace log dir: %s", trace_log_dir.string().c_str());
  DC_LOG_INFO("  Trace file path: %s", trace_file_path.string().c_str());
  DC_LOG_INFO("  Trace dir pattern: %s",
              trace_dir_pattern.empty() ? "<none>" : trace_dir_pattern.c_str());
  DC_LOG_INFO("  Data dir: %s", data_dir.string().c_str());
  DC_LOG_INFO("  Probe file path: %s", probe_file_path.string().c_str());
  DC_LOG_INFO("  System probe path: %s", system_probe_path.string().c_str());
  DC_LOG_INFO("  Invalid probe database: %s", invalid_probe_file_path.string().c_str());
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

  const std::string generated_file_suffix =
      user + "-" + run_id + "-" + hostname + "-" + config_name;
  const std::string lookup_file_suffix = std::string(DATACRUMBS_INSTALL_USER) + "-" + config_name;
  trace_file_path = trace_log_dir / ("trace-" + generated_file_suffix + ".pfw.gz");
  invalid_probe_file_path = data_dir / ("probes-invalid-" + lookup_file_suffix + ".json.gz");
  server_ready_file = server_run_dir / ("datacrumbs-" + run_id + ".ready");
}

void RuntimeConfigurationManager::load_runtime_system_configuration() {
  const std::string payload = datacrumbs::probe_file::read_probe_payload(system_probe_path);
  if (payload.empty()) {
    throw std::runtime_error("Failed to read system probe file: " + system_probe_path.string());
  }

  json_object* root = json_tokener_parse(payload.c_str());
  if (!root || json_object_get_type(root) != json_type_object) {
    if (root) json_object_put(root);
    throw std::runtime_error("Failed to parse system probe file: " + system_probe_path.string());
  }

  json_object* summary = nullptr;
  json_object* system_configuration = nullptr;
  if (json_object_object_get_ex(root, "summary", &summary) &&
      json_object_get_type(summary) == json_type_object) {
    const std::string imported_config_path = json_string_or_empty(summary, "config_path");
    const std::string imported_trace_dir = json_string_or_empty(summary, "trace_log_dir");
    const std::string imported_config_name = json_string_or_empty(summary, "config_name");
    if (!imported_config_path.empty()) config_path = imported_config_path;
    if (trace_dir_pattern.empty() && !imported_trace_dir.empty())
      trace_log_dir = imported_trace_dir;
    if (!imported_config_name.empty()) config_name = imported_config_name;
  }

  if (json_object_object_get_ex(root, "system_configuration", &system_configuration) &&
      json_object_get_type(system_configuration) == json_type_object) {
    const std::string imported_log_dir =
        json_string_or_empty(system_configuration, "DATACRUMBS_LOG_DIR");
    const std::string imported_data_dir =
        json_string_or_empty(system_configuration, "DATACRUMBS_INSTALL_DATA_DIR");
    const std::string imported_inclusion_paths =
        json_string_or_empty(system_configuration, "DATACRUMBS_INCLUSION_PATHS");
    const std::string imported_trace_dir_pattern =
        json_string_or_empty(system_configuration, "DATACRUMBS_TRACE_DIR_PATTERN");
    const std::string imported_run_dir =
        json_string_or_empty(system_configuration, "DATACRUMBS_SERVER_RUN_DIR");
    const std::string imported_run_id_file =
        json_string_or_empty(system_configuration, "DATACRUMBS_SERVER_RUN_ID_FILE");

    if (!imported_log_dir.empty()) log_dir = imported_log_dir;
    if (!imported_data_dir.empty()) data_dir = imported_data_dir;
    if (!imported_trace_dir_pattern.empty()) trace_dir_pattern = imported_trace_dir_pattern;
    if (!imported_run_dir.empty()) server_run_dir = imported_run_dir;
    if (!imported_run_id_file.empty()) server_run_id_file = imported_run_id_file;
    if (inclusion_paths.empty() && !imported_inclusion_paths.empty()) {
      inclusion_paths = imported_inclusion_paths;
    }
  }

  json_object_put(root);
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
  load_invalid_runtime_probes();

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
      if (is_known_invalid_runtime_probe(probe->name, function_name)) {
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
        skipped_known_invalid_functions, invalid_probe_file_path.string().c_str());
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

void RuntimeConfigurationManager::load_invalid_runtime_probes() {
  if (invalid_probe_file_path.empty() || !std::filesystem::exists(invalid_probe_file_path)) {
    return;
  }

  const std::string content = datacrumbs::probe_file::read_probe_payload(invalid_probe_file_path);
  if (content.empty()) {
    DC_LOG_WARN("[RuntimeConfigurationManager] Failed to read invalid probe database: %s",
                invalid_probe_file_path.string().c_str());
    return;
  }
  json_object* root = json_tokener_parse(content.c_str());
  if (!root || json_object_get_type(root) != json_type_array) {
    if (root) json_object_put(root);
    DC_LOG_WARN("[RuntimeConfigurationManager] Invalid invalid probe database format: %s",
                invalid_probe_file_path.string().c_str());
    return;
  }

  const int arr_len = json_object_array_length(root);
  for (int i = 0; i < arr_len; ++i) {
    json_object* probe_obj = json_object_array_get_idx(root, i);
    if (!probe_obj) continue;

    json_object* name_obj = nullptr;
    json_object* functions_obj = nullptr;
    if (!json_object_object_get_ex(probe_obj, "name", &name_obj) ||
        !json_object_object_get_ex(probe_obj, "functions", &functions_obj) ||
        json_object_get_type(name_obj) != json_type_string ||
        json_object_get_type(functions_obj) != json_type_array) {
      continue;
    }

    auto& function_set = invalid_runtime_probes[std::string(json_object_get_string(name_obj))];
    const int function_count = json_object_array_length(functions_obj);
    for (int j = 0; j < function_count; ++j) {
      json_object* function_obj = json_object_array_get_idx(functions_obj, j);
      if (function_obj && json_object_get_type(function_obj) == json_type_string) {
        function_set.insert(json_object_get_string(function_obj));
      }
    }
  }

  json_object_put(root);
}

bool RuntimeConfigurationManager::is_known_invalid_runtime_probe(
    const std::string& probe_name, const std::string& function_name) const {
  const auto probe_it = invalid_runtime_probes.find(probe_name);
  if (probe_it == invalid_runtime_probes.end()) {
    return false;
  }
  return probe_it->second.find(function_name) != probe_it->second.end();
}

void RuntimeConfigurationManager::record_invalid_runtime_probe(const std::string& probe_name,
                                                               const std::string& function_name) {
  invalid_runtime_probes[probe_name].insert(function_name);
}

void RuntimeConfigurationManager::persist_invalid_runtime_probes() const {
  if (invalid_probe_file_path.empty()) {
    return;
  }

  std::error_code ec;
  std::filesystem::create_directories(invalid_probe_file_path.parent_path(), ec);

  json_object* root = json_object_new_array();
  for (const auto& [probe_name, functions] : invalid_runtime_probes) {
    if (functions.empty()) {
      continue;
    }
    json_object* probe_obj = json_object_new_object();
    json_object_object_add(probe_obj, "name", json_object_new_string(probe_name.c_str()));
    json_object* function_array = json_object_new_array();
    std::vector<std::string> sorted_functions(functions.begin(), functions.end());
    std::sort(sorted_functions.begin(), sorted_functions.end());
    for (const auto& function_name : sorted_functions) {
      json_object_array_add(function_array, json_object_new_string(function_name.c_str()));
    }
    json_object_object_add(probe_obj, "functions", function_array);
    json_object_array_add(root, probe_obj);
  }

  const std::string payload = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
  if (!datacrumbs::probe_file::write_gzip_file(invalid_probe_file_path, payload)) {
    json_object_put(root);
    DC_LOG_WARN("[RuntimeConfigurationManager] Failed to persist invalid probe database: %s",
                invalid_probe_file_path.string().c_str());
    return;
  }

  if (struct passwd* pwd = getpwnam(DATACRUMBS_INSTALL_USER); pwd != nullptr) {
    chown(invalid_probe_file_path.c_str(), pwd->pw_uid, pwd->pw_gid);
  }
  chmod(invalid_probe_file_path.c_str(), S_IRUSR | S_IWUSR);
  json_object_put(root);
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
