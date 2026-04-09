#include "datacrumbs/system_configurator/system_configurator.h"

#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/probe_file.h>
#include <datacrumbs/datacrumbs_config.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

constexpr int kSqliteBusyTimeoutMs = 5000;

bool sqlite_exec(sqlite3* db, const char* sql, const std::string& database_path,
                 const char* operation) {
  char* error_message = nullptr;
  const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error_message);
  if (rc == SQLITE_OK) {
    return true;
  }

  const char* sqlite_message = error_message ? error_message : sqlite3_errmsg(db);
  DC_LOG_ERROR("Failed to %s %s: %s", operation, database_path.c_str(),
               sqlite_message ? sqlite_message : "unknown sqlite error");
  if (error_message != nullptr) {
    sqlite3_free(error_message);
  }
  return false;
}

bool write_kv_table(sqlite3* db, const char* table_name,
                    const std::unordered_map<std::string, std::string>& values,
                    const std::string& database_path) {
  const std::string sql =
      std::string("INSERT OR REPLACE INTO ") + table_name + " (key, value) VALUES (?, ?);";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
    DC_LOG_ERROR("Failed to prepare insert into %s for %s: %s", table_name, database_path.c_str(),
                 sqlite3_errmsg(db));
    return false;
  }

  bool ok = true;
  for (const auto& [key, value] : values) {
    sqlite3_reset(statement);
    sqlite3_clear_bindings(statement);
    sqlite3_bind_text(statement, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) != SQLITE_DONE) {
      DC_LOG_ERROR("Failed to insert key '%s' into %s for %s: %s", key.c_str(), table_name,
                   database_path.c_str(), sqlite3_errmsg(db));
      ok = false;
      break;
    }
  }

  sqlite3_finalize(statement);
  return ok;
}

bool configure_generated_database(sqlite3* db, const std::string& database_path) {
  return sqlite_exec(db, "PRAGMA journal_mode=MEMORY;", database_path, "set journal_mode") &&
         sqlite_exec(db, "PRAGMA synchronous=OFF;", database_path, "set synchronous") &&
         sqlite_exec(db, "PRAGMA temp_store=MEMORY;", database_path, "set temp_store") &&
         sqlite_exec(db, "PRAGMA locking_mode=EXCLUSIVE;", database_path, "set locking_mode");
}

}  // namespace

std::string SystemConfigurator::current_hostname() const {
  char hostname_buf[256] = {0};
  if (gethostname(hostname_buf, sizeof(hostname_buf) - 1) != 0) {
    throw std::runtime_error("Failed to get hostname.");
  }
  return hostname_buf;
}

std::string SystemConfigurator::system_configuration_path() const {
  const char* override_path = std::getenv("DATACRUMBS_SYSTEM_PROBE_FILE_OVERRIDE");
  if (override_path != nullptr && override_path[0] != '\0') {
    return std::string(override_path);
  }
  return DATACRUMBS_SYSTEM_PROBE_FILE;
}

std::string SystemConfigurator::optional_macro_string(const char* value) const {
  return value != nullptr ? std::string(value) : std::string();
}

void SystemConfigurator::enforce_secret_ownership_and_mode() const {
  const auto secret_path = datacrumbs::probe_file::secret_path();
  chown(secret_path.c_str(), 0, 0);
  chmod(secret_path.c_str(), S_IRUSR);
}

std::unordered_map<std::string, std::string> SystemConfigurator::system_configuration_values()
    const {
  std::string inclusion_paths;
  std::string probe_signing_disabled = "0";
#ifdef DATACRUMBS_INCLUSION_PATHS
  inclusion_paths = optional_macro_string(DATACRUMBS_INCLUSION_PATHS);
#endif
#ifdef DATACRUMBS_DISABLE_PROBE_SIGNING
  probe_signing_disabled = "1";
#endif

  return {
      {"DATACRUMBS_INSTALL_PREFIX", DATACRUMBS_INSTALL_PREFIX_PATH},
      {"DATACRUMBS_INSTALL_HOST", DATACRUMBS_INSTALL_HOST},
      {"DATACRUMBS_INSTALL_USER", DATACRUMBS_INSTALL_USER},
      {"DATACRUMBS_INSTALL_BIN_DIR", DATACRUMBS_INSTALL_BIN_DIR},
      {"DATACRUMBS_INSTALL_SBIN_DIR", DATACRUMBS_INSTALL_SBIN_DIR},
      {"DATACRUMBS_INSTALL_LIB_DIR", DATACRUMBS_INSTALL_LIB_DIR},
      {"DATACRUMBS_INSTALL_LIBEXEC_DIR", DATACRUMBS_INSTALL_LIBEXEC_DIR},
      {"DATACRUMBS_INSTALL_ETC_DIR", DATACRUMBS_INSTALL_ETC_DIR},
      {"DATACRUMBS_INSTALL_CONFIGS_DIR", DATACRUMBS_INSTALL_CONFIGS_DIR},
      {"DATACRUMBS_INSTALL_DATA_DIR", DATACRUMBS_INSTALL_SHARED_DATA_DIR},
      {"DATACRUMBS_INSTALL_MODULES_DIR", DATACRUMBS_INSTALL_MODULES_DIR},
      {"DATACRUMBS_INSTALL_PROBE_OBJECTS_DIR", DATACRUMBS_INSTALL_PROBE_OBJECTS_DIR},
      {"DATACRUMBS_INSTALL_RUNSTATEDIR", DATACRUMBS_INSTALL_RUNSTATEDIR},
      {"DATACRUMBS_CLIENT_LIB", DATACRUMBS_CLIENT_LIB},
      {"DATACRUMBS_TRACE_DIR_PATTERN", DATACRUMBS_TRACE_DIR_PATTERN},
      {"DATACRUMBS_JOB_SCHEDULER", DATACRUMBS_JOB_SCHEDULER},
      {"DATACRUMBS_JOB_ID_VAR", DATACRUMBS_JOB_ID_VAR},
      {"DATACRUMBS_SERVER_RUN_DIR", DATACRUMBS_SERVER_RUN_DIR},
      {"DATACRUMBS_SERVER_RUN_ID_FILE", DATACRUMBS_SERVER_RUN_ID_FILE},
      {"DATACRUMBS_LOG_DIR", DATACRUMBS_LOG_DIR},
      {"DATACRUMBS_USER", DATACRUMBS_USER},
      {"DATACRUMBS_INCLUSION_PATHS", inclusion_paths},
      {"DATACRUMBS_KERNEL_UNAME_R", DATACRUMBS_KERNEL_UNAME_R},
      {"DATACRUMBS_KERNEL_HEADERS_PATH", DATACRUMBS_KERNEL_HEADERS_PATH},
      {"DATACRUMBS_LIBC_SO", DATACRUMBS_LIBC_SO},
      {"DATACRUMBS_DISABLE_PROBE_SIGNING", probe_signing_disabled},
  };
}

int SystemConfigurator::run() {
  try {
    bool secret_ready = false;
    std::string secret;
    if (datacrumbs::probe_file::ensure_probe_secret(&secret)) {
      enforce_secret_ownership_and_mode();
      secret_ready = true;
    } else if (geteuid() == 0) {
      DC_LOG_ERROR("Failed to create probe signing secret");
      return 1;
    } else {
      DC_LOG_WARN("Skipping probe signing secret creation during non-root configuration pass");
    }

    const std::string configuration_path = system_configuration_path();
    if (std::filesystem::exists(configuration_path)) {
      if (chmod(configuration_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
        DC_LOG_ERROR("Failed to update system configuration file permissions: %s",
                     configuration_path.c_str());
        return 1;
      }
      DC_LOG_INFO("System configuration already exists at: %s", configuration_path.c_str());
      if (secret_ready) {
        DC_LOG_INFO("Probe signing secret ensured at: %s",
                    datacrumbs::probe_file::secret_path().string().c_str());
      }
      return 0;
    }

    const std::string hostname = current_hostname();
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(configuration_path).parent_path(),
                                        ec);
    if (ec) {
      DC_LOG_ERROR("Failed to create system configuration directory for %s: %s",
                   configuration_path.c_str(), ec.message().c_str());
      return 1;
    }

    sqlite3* db = nullptr;
    if (sqlite3_open(configuration_path.c_str(), &db) != SQLITE_OK) {
      DC_LOG_ERROR("Failed to open system configuration database: %s", configuration_path.c_str());
      if (db != nullptr) {
        sqlite3_close(db);
      }
      return 1;
    }
    sqlite3_busy_timeout(db, kSqliteBusyTimeoutMs);
    if (!configure_generated_database(db, configuration_path)) {
      sqlite3_close(db);
      return 1;
    }

    const auto summary_values = std::unordered_map<std::string, std::string>{
        {"trace_log_dir", DATACRUMBS_CONFIGURED_TRACE_DIR},
        {"data_dir", DATACRUMBS_INSTALL_SHARED_DATA_DIR},
        {"system_probe_path", configuration_path},
        {"hostname", hostname},
        {"user", DATACRUMBS_USER},
        {"install_user", DATACRUMBS_INSTALL_USER},
    };

    const bool transaction_started =
        sqlite_exec(db, "BEGIN IMMEDIATE TRANSACTION;", configuration_path, "begin transaction");
    const bool ok =
        transaction_started &&
        sqlite_exec(db,
                    "CREATE TABLE IF NOT EXISTS system_configuration ("
                    "key TEXT PRIMARY KEY,"
                    "value TEXT NOT NULL"
                    ");",
                    configuration_path, "create system_configuration table") &&
        sqlite_exec(db,
                    "CREATE TABLE IF NOT EXISTS summary ("
                    "key TEXT PRIMARY KEY,"
                    "value TEXT NOT NULL"
                    ");",
                    configuration_path, "create summary table") &&
        sqlite_exec(db, "DELETE FROM system_configuration;", configuration_path,
                    "clear system_configuration table") &&
        sqlite_exec(db, "DELETE FROM summary;", configuration_path, "clear summary table") &&
        write_kv_table(db, "system_configuration", system_configuration_values(),
                       configuration_path) &&
        write_kv_table(db, "summary", summary_values, configuration_path) &&
        sqlite_exec(db, "COMMIT;", configuration_path, "commit transaction");

    if (!ok) {
      if (transaction_started) {
        sqlite_exec(db, "ROLLBACK;", configuration_path, "rollback transaction");
      }
      sqlite3_close(db);
      DC_LOG_ERROR("Failed to write system configuration database: %s", configuration_path.c_str());
      return 1;
    }
    sqlite3_close(db);

    if (chmod(configuration_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
      DC_LOG_ERROR("Failed to update system configuration file permissions: %s",
                   configuration_path.c_str());
      return 1;
    }

    DC_LOG_INFO("System configuration written to: %s", configuration_path.c_str());
    if (secret_ready) {
      DC_LOG_INFO("Probe signing secret ensured at: %s",
                  datacrumbs::probe_file::secret_path().string().c_str());
    }
    return 0;
  } catch (const std::exception& ex) {
    DC_LOG_ERROR("System configurator failed: %s", ex.what());
    return 1;
  }
}
