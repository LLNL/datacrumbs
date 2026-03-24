#include "datacrumbs/system_configurator/system_configurator.h"

#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/probe_file.h>
#include <datacrumbs/datacrumbs_config.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>

std::string SystemConfigurator::current_hostname() const {
  char hostname_buf[256] = {0};
  if (gethostname(hostname_buf, sizeof(hostname_buf) - 1) != 0) {
    throw std::runtime_error("Failed to get hostname.");
  }
  return hostname_buf;
}

void SystemConfigurator::add_string(json_object* root, const char* key,
                                    const std::string& value) const {
  json_object_object_add(root, key, json_object_new_string(value.c_str()));
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

void SystemConfigurator::add_system_configuration(json_object* root) const {
  std::string job_other_flags;
  std::string inclusion_paths;
  std::string probe_signing_disabled = "0";
#ifdef DATACRUMBS_JOB_OTHER_FLAGS
  job_other_flags = optional_macro_string(DATACRUMBS_JOB_OTHER_FLAGS);
#endif
#ifdef DATACRUMBS_INCLUSION_PATHS
  inclusion_paths = optional_macro_string(DATACRUMBS_INCLUSION_PATHS);
#endif
#ifdef DATACRUMBS_DISABLE_PROBE_SIGNING
  probe_signing_disabled = "1";
#endif

  const std::unordered_map<std::string, std::string> values = {
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
      {"DATACRUMBS_JOB_OTHER_FLAGS", job_other_flags},
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

  for (const auto& [key, value] : values) {
    add_string(root, key.c_str(), value);
  }
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

    const std::string system_configuration_path = this->system_configuration_path();
    if (std::filesystem::exists(system_configuration_path)) {
      if (chmod(system_configuration_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
        DC_LOG_ERROR("Failed to update system configuration file permissions: %s",
                     system_configuration_path.c_str());
        return 1;
      }
      DC_LOG_INFO("System configuration already exists at: %s",
                  system_configuration_path.c_str());
      if (secret_ready) {
        DC_LOG_INFO("Probe signing secret ensured at: %s",
                    datacrumbs::probe_file::secret_path().string().c_str());
      }
      return 0;
    }

    const std::string hostname = current_hostname();

    json_object* root = json_object_new_object();
    json_object* summary = json_object_new_object();
    json_object* system_configuration = json_object_new_object();

    add_system_configuration(system_configuration);
    json_object_object_add(root, "system_configuration", system_configuration);

    add_string(summary, "config_name", hostname);
    add_string(summary, "config_path", DATACRUMBS_INSTALL_CONFIGS_DIR);
    add_string(summary, "trace_log_dir", DATACRUMBS_CONFIGURED_TRACE_DIR);
    add_string(summary, "data_dir", DATACRUMBS_INSTALL_SHARED_DATA_DIR);
    add_string(summary, "system_probe_path", system_configuration_path);
    add_string(summary, "hostname", hostname);
    add_string(summary, "user", DATACRUMBS_USER);
    add_string(summary, "install_user", DATACRUMBS_INSTALL_USER);
    json_object_object_add(root, "summary", summary);

    const char* json_payload = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    const bool ok =
        datacrumbs::probe_file::write_gzip_file(system_configuration_path, json_payload);
    json_object_put(root);

    if (!ok) {
      DC_LOG_ERROR("Failed to write system configuration file: %s",
                   system_configuration_path.c_str());
      return 1;
    }
    if (chmod(system_configuration_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
      DC_LOG_ERROR("Failed to update system configuration file permissions: %s",
                   system_configuration_path.c_str());
      return 1;
    }

    DC_LOG_INFO("System configuration written to: %s", system_configuration_path.c_str());
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
