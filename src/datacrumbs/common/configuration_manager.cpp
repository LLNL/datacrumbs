/**
 * @file configuration_manager.cpp
 * @brief Implements the ConfigurationManager class for managing DataCrumbs configuration.
 *
 * This file contains the implementation of the ConfigurationManager class, which is responsible
 * for parsing command-line arguments, loading YAML configuration files, and setting up
 * configuration parameters for the DataCrumbs application. It also includes the ArgumentParser
 * class for handling command-line arguments and utility functions for deriving and validating
 * configuration values.
 */

/**
 * std headers
 */
#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
/**
 * Internal headers
 */
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/enumerations.h>
#include <datacrumbs/common/logging.h>  // <-- Added logging header
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/utils.h>
/**
 * External headers
 */
#include <mpi.h>
#include <yaml-cpp/yaml.h>
#include <datacrumbs/utilities/config/yaml_utils.h>

namespace datacrumbs {

// Singleton template specialization for ConfigurationManager
template <>
std::shared_ptr<datacrumbs::ConfigurationManager>
    datacrumbs::Singleton<datacrumbs::ConfigurationManager>::instance = nullptr;
template <>
bool datacrumbs::Singleton<datacrumbs::ConfigurationManager>::stop_creating_instances = false;

/**
 * YAML keys for configuration
 */
#define DC_YAML_TRACE_LOG_DIR "trace_log_dir"
#define DC_YAML_DATA_DIR "data_dir"
#define DC_YAML_CAPTURE_PROBES "capture_probes"
#define DC_YAML_USER "user"
#define DC_YAML_INCLUSION_PATH "inclusion_path"

ArgumentParser::ArgumentParser(int argc, char** argv, ExecutableType exe_type) {
  DC_LOG_TRACE("[ArgumentParser] Parsing command line arguments...");
  if (exe_type == ExecutableType::SIMPLE && argc < 2) {
    throw std::invalid_argument("Configuration name is required as the first argument.");
  } else if (exe_type == ExecutableType::DAEMON && argc < 3) {
    throw std::invalid_argument(
        "Executable mode (start, stop, or run) and configuration name are required as the first "
        "two arguments.");
  }
  int start_index = 1;
  if (exe_type == ExecutableType::SIMPLE) {
    config_name = argv[start_index++];
  } else if (exe_type == ExecutableType::DAEMON) {
    ExecutableMode exe_mode;
    convert(argv[start_index++], exe_mode);
    this->exe_mode = exe_mode;
    config_name = argv[start_index++];
  } else {
    throw std::invalid_argument("Unknown ExecutableType.");
  }

  for (int i = start_index; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--run_id" && i + 1 < argc) {
      run_id = argv[++i];
      DC_LOG_DEBUG("[ArgumentParser] Run ID set to: %s", run_id->c_str());
    } else if (arg == "--trace_log_dir" && i + 1 < argc) {
      trace_log_dir = argv[++i];
      DC_LOG_DEBUG("[ArgumentParser] Trace log dir set to: %s", trace_log_dir->c_str());
    } else if (arg == "--data_dir" && i + 1 < argc) {
      data_dir = argv[++i];
      DC_LOG_DEBUG("[ArgumentParser] Data directory set to: %s", data_dir->c_str());
    } else if (arg == "--config_path" && i + 1 < argc) {
      config_path = argv[++i];
      DC_LOG_DEBUG("[ArgumentParser] Config path set to: %s", config_path->c_str());
    } else if (arg == "--user" && i + 1 < argc) {
      user = argv[++i];
      DC_LOG_DEBUG("[ArgumentParser] User set to: %s", user->c_str());
    } else if (arg == "--inclusion_path" && i + 1 < argc) {
      inclusion_path = argv[++i];
      DC_LOG_DEBUG("[ArgumentParser] Inclusion path set to: %s", inclusion_path->c_str());
    } else if (arg == "--log_dir" && i + 1 < argc) {
      log_dir = argv[++i];
      DC_LOG_DEBUG("[ArgumentParser] Log directory set to: %s", log_dir->c_str());
    } else if (arg == "--disable-mpi") {
      disable_mpi = true;
      DC_LOG_DEBUG("[ArgumentParser] disable_mpi set to: %s", disable_mpi ? "true" : "false");
    } else if (arg == "--help" || arg == "-h") {
      DC_LOG_PRINT(
          "Usage: %s <config_name> [--run_id <id>] [--trace_log_dir <path>] "
          "[--config_path <path>] [--user <user>] [--data_dir "
          "<path>] [--inclusion_path <path>] [--log_dir <path>] [--disable-mpi]",
          argv[0]);
      exit(0);
    } else {
      DC_LOG_ERROR("[ArgumentParser] Unknown argument: %s", arg.c_str());
      throw std::invalid_argument("Unknown argument: " + arg);
    }
  }
}

/**
 * @brief ConfigurationManager constructor.
 *
 * Initializes the ConfigurationManager with command-line arguments, loads the YAML configuration
 * file, parses it, and sets up the necessary configurations. Also derives and validates
 * configurations.
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 */
ConfigurationManager::ConfigurationManager(int argc, char** argv, bool print,
                                           ExecutableType exe_type)
    : path(DATACRUMBS_CONFIG_PATH),
      exe_mode(ExecutableMode::RUN),
      name("default"),
      trace_log_dir(DATACRUMBS_LOG_DIR),
      capture_probes(),
      user("datacrumbs"),
      run_id("0") {
  struct rlimit rl;
  if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
    rl.rlim_cur = rl.rlim_max;  // Set soft limit to hard limit
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
      DC_LOG_WARN("[ConfigurationManager] Failed to set ulimit -n to hard limit.");
    } else {
      DC_LOG_DEBUG("[ConfigurationManager] Set ulimit -n to hard limit: %lu", rl.rlim_max);
    }
  } else {
    DC_LOG_WARN("[ConfigurationManager] Failed to get current ulimit -n.");
  }
  if (getrlimit(RLIMIT_MEMLOCK, &rl) == 0) {
    rl.rlim_cur = rl.rlim_max;  // Set soft limit to hard limit
    if (setrlimit(RLIMIT_MEMLOCK, &rl) != 0) {
      DC_LOG_WARN("[ConfigurationManager] Failed to set ulimit -l to hard limit.");
    } else {
      DC_LOG_DEBUG("[ConfigurationManager] Set ulimit -l to hard limit: %lu", rl.rlim_max);
    }
  } else {
    DC_LOG_WARN("[ConfigurationManager] Failed to get current ulimit -l.");
  }
  // Set ulimit -c (core file size) to its hard limit
  if (getrlimit(RLIMIT_CORE, &rl) == 0) {
    rl.rlim_cur = rl.rlim_max;  // Set soft limit to hard limit
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
      DC_LOG_WARN("[ConfigurationManager] Failed to set ulimit -c to hard limit.");
    } else {
      DC_LOG_DEBUG("[ConfigurationManager] Set ulimit -c to hard limit: %lu", rl.rlim_max);
    }
  } else {
    DC_LOG_WARN("[ConfigurationManager] Failed to get current ulimit -c.");
  }
  DC_LOG_TRACE("[ConfigurationManager] Initializing with arguments...");
  ArgumentParser parser(argc, argv, exe_type);
  this->name = parser.config_name;
  // Override config path if provided as argument
  if (parser.config_path) {
    this->path = *parser.config_path;
    DC_LOG_DEBUG("[ConfigurationManager] Config path overridden by argument: %s",
                 this->path.string().c_str());
  }
  YAML::Node config;
  std::filesystem::path config_path = this->path / (this->name + ".yaml");
  DC_LOG_DEBUG("[ConfigurationManager] Loading configuration file: %s",
               config_path.string().c_str());
  try {
    config = YAML::LoadFile(config_path.string());
    DC_LOG_DEBUG("[ConfigurationManager] Configuration file loaded successfully.");
  } catch (const YAML::ParserException& e) {
    DC_LOG_ERROR("[ConfigurationManager] Failed to parse configuration file: %s",
                 config_path.string().c_str());
    throw std::runtime_error("Failed to parse configuration file: " + config_path.string());
  } catch (const YAML::BadFile& e) {
    DC_LOG_ERROR("[ConfigurationManager] Failed to load configuration file: %s",
                 config_path.string().c_str());
    throw std::runtime_error("Failed to load configuration file: " + config_path.string());
  }

  // Parse YAML configuration if loaded successfully
  if (config) {
    DC_LOG_TRACE("[ConfigurationManager] Parsing configuration YAML...");
    // Parse trace log directory from YAML
    if (config[DC_YAML_TRACE_LOG_DIR]) {
      this->trace_log_dir = config[DC_YAML_TRACE_LOG_DIR].as<std::string>();
      DC_LOG_DEBUG("[ConfigurationManager] Trace log dir set from config: %s",
                   this->trace_log_dir.string().c_str());
    }
    // Parse data directory from YAML or use default
    if (config[DC_YAML_DATA_DIR]) {
      this->data_dir = config[DC_YAML_DATA_DIR].as<std::string>();
      DC_LOG_DEBUG("[ConfigurationManager] Data directory set from config: %s",
                   this->data_dir.string().c_str());
    } else {
      this->data_dir = DATACRUMBS_DATA_DIR;
      DC_LOG_DEBUG("[ConfigurationManager] Data directory not specified, using default: %s",
                   this->data_dir.string().c_str());
    }
    // Parse capture probes from YAML via shared utility (skips disabled)
    if (config[DC_YAML_CAPTURE_PROBES]) {
      DC_LOG_TRACE("[ConfigurationManager] Parsing capture probes via utilities...");
      auto parsed = datacrumbs::utilities::capture_probes(config_path.string());
      this->capture_probes.insert(this->capture_probes.end(), parsed.begin(), parsed.end());
    }
    // Parse user from YAML or use default
    if (config[DC_YAML_USER]) {
      this->user = config[DC_YAML_USER].as<std::string>();
      DC_LOG_DEBUG("[ConfigurationManager] User set from config: %s", this->user.c_str());
    } else {
      DC_LOG_DEBUG("[ConfigurationManager] No user specified in config, using default: %s",
                   this->user.c_str());
    }
    // Parse inclusion path from YAML
    if (config[DC_YAML_INCLUSION_PATH]) {
      this->inclusion_path = config[DC_YAML_INCLUSION_PATH].as<std::string>();
      DC_LOG_DEBUG("[ConfigurationManager] Inclusion path set from config: %s",
                   this->inclusion_path.c_str());
    }
    // Override run_id if provided as argument
    if (parser.run_id) {
      this->run_id = *parser.run_id;
      DC_LOG_DEBUG("[ConfigurationManager] Run ID overridden by argument: %s",
                   this->run_id.c_str());
    }
    if (parser.exe_mode) {
      this->exe_mode = *parser.exe_mode;
      DC_LOG_DEBUG("[ConfigurationManager] Executable mode set from argument: %d",
                   static_cast<int>(this->exe_mode));
    }
    // Override config path if provided as argument
    if (parser.data_dir) {
      this->data_dir = *parser.data_dir;
      DC_LOG_DEBUG("[ConfigurationManager] Data directory overridden by argument: %s",
                   this->data_dir.string().c_str());
    }
    // Override trace log dir if provided as argument
    if (parser.trace_log_dir) {
      this->trace_log_dir = *parser.trace_log_dir;
      DC_LOG_DEBUG("[ConfigurationManager] Trace log dir overridden by argument: %s",
                   parser.trace_log_dir->c_str());
    }
    // Override user if provided as argument
    if (parser.user) {
      this->user = *parser.user;
      DC_LOG_DEBUG("[ConfigurationManager] User overridden by argument: %s", parser.user->c_str());
    } else {
      DC_LOG_DEBUG("[ConfigurationManager] No user specified, using default: %s",
                   this->user.c_str());
    }
    // Override inclusion path if provided as argument
    if (parser.inclusion_path) {
      this->inclusion_path = *parser.inclusion_path;
      DC_LOG_DEBUG("[ConfigurationManager] Inclusion path overridden by argument: %s",
                   parser.inclusion_path->c_str());
    }
    // Override log dir if provided as argument
    if (parser.log_dir) {
      this->log_dir = *parser.log_dir;
      DC_LOG_DEBUG("[ConfigurationManager] Log directory overridden by argument: %s",
                   parser.log_dir->c_str());
    } else {
      this->log_dir = std::filesystem::current_path();
      DC_LOG_DEBUG("[ConfigurationManager] No log directory specified, using default: %s",
                   this->log_dir.c_str());
    }
    // Override disable_mpi if provided as argument
    if (parser.disable_mpi) {
      this->disable_mpi = *parser.disable_mpi;
      DC_LOG_DEBUG("[ConfigurationManager] disable_mpi overridden by argument: %s",
                   this->disable_mpi ? "true" : "false");
    } else {
      this->disable_mpi = false;
      DC_LOG_DEBUG("[ConfigurationManager] No disable_mpi specified, using default: false");
    }
  }
  // Derive additional configuration values and validate
  derive_configurations();
  validate_configurations();
  if (print) {
    print_configurations();
    DC_LOG_INFO("[ConfigurationManager] Initialization complete.");
  }
};

void ConfigurationManager::print_configurations() {
  // Log final configuration for debugging
  DC_LOG_INFO("[ConfigurationManager] Final configuration:");
  DC_LOG_INFO("[ConfigurationManager] Capture probes loaded: %zu", this->capture_probes.size());
  DC_LOG_INFO("[ConfigurationManager] Category map loaded with %zu entries.", category_map.size());
  DC_LOG_INFO("  Path: %s", this->path.string().c_str());
  DC_LOG_INFO("  Name: %s", this->name.c_str());
  DC_LOG_INFO("  Trace log dir: %s", this->trace_log_dir.string().c_str());
  DC_LOG_INFO("  Trace file path: %s", this->trace_file_path.string().c_str());
  DC_LOG_INFO("  Data dir: %s", this->data_dir.string().c_str());
  DC_LOG_INFO("  Probe file path: %s", this->probe_file_path.string().c_str());
  DC_LOG_INFO("  Probe exclusion file path: %s", this->probe_exclusion_file_path.string().c_str());
  DC_LOG_INFO("  Probe invalid file path: %s", this->probe_invalid_file_path.string().c_str());
  DC_LOG_INFO("  Manual probe path: %s", this->manual_probe_path.string().c_str());
  DC_LOG_INFO("  Category map path: %s", this->category_map_path.string().c_str());
  DC_LOG_INFO("  Profiling interval: %f", DATACRUMBS_TIME_INTERVAL_NS / 1e9);
  DC_LOG_INFO("  Runtime User: %s", this->user.c_str());
  DC_LOG_INFO("  Install user: %s", DATACRUMBS_INSTALL_USER);
  DC_LOG_INFO("  Hostname: %s", this->hostname.c_str());
  DC_LOG_INFO("  Capture probes: %d", static_cast<int>(this->capture_probes.size()));
  if (DATACRUMBS_MODE == 1) {
    DC_LOG_INFO("  Mode: Tracing");
  } else if (DATACRUMBS_MODE == 2) {
    DC_LOG_INFO("  Mode: Profiling");
  }
  if (this->inclusion_path.empty()) {
    DC_LOG_INFO("  Inclusion path: Not set");
  } else {
    DC_LOG_INFO("  Inclusion path: %s", this->inclusion_path.c_str());
  }
  for (const auto& probe : this->capture_probes) {
    DC_LOG_INFO("    Probe: name=%s, type=%d, probe_type=%d, regex=%s", probe->name.c_str(),
                static_cast<int>(probe->type), static_cast<int>(probe->probe_type),
                probe->regex.c_str());
  }
}

/**
 * @brief Derives additional configuration values based on current settings.
 *
 * This function generates file paths for trace files, probe files, exclusion files,
 * and category maps based on the hostname, process ID, timestamp, and user.
 */
void ConfigurationManager::derive_configurations() {
  DC_LOG_TRACE("[ConfigurationManager] Deriving configurations...");
  DC_LOG_DEBUG("[ConfigurationManager] Process ID: %d for rank: %d", getpid(), this->mpi_rank);

  // Use this->hostname (std::string) instead of local char array
  std::string hostname;
  char hostname_buf[256] = {0};
  if (gethostname(hostname_buf, sizeof(hostname_buf) - 1) != 0) {
    DC_LOG_ERROR("[ConfigurationManager] Failed to get hostname.");
    throw std::runtime_error("Failed to get hostname.");
  }
  hostname = hostname_buf;
  this->hostname = hostname;
  DC_LOG_DEBUG("[ConfigurationManager] Hostname: %s for rank: %d", this->hostname.c_str(),
               this->mpi_rank);

  std::string generated_file_suffix;
  if (this->disable_mpi) {
    generated_file_suffix = this->user + "-" + this->run_id + "-" + hostname + "-" + this->name;
  } else {
    generated_file_suffix = this->user + "-" + this->run_id + "-" + std::to_string(this->mpi_rank) +
                            "-" + std::to_string(this->mpi_size) + "-" + this->name;
  }

  std::string trace_file_name = "trace-" + generated_file_suffix + ".pfw.gz";
  this->trace_file_path = this->trace_log_dir / trace_file_name;
  DC_LOG_DEBUG("[ConfigurationManager] Trace file path: %s for rank: %d",
               this->trace_file_path.string().c_str(), this->mpi_rank);

  std::string hostname_str(this->name);
  // Remove digits from hostname for file naming
  hostname_str.erase(std::remove_if(hostname_str.begin(), hostname_str.end(), ::isdigit),
                     hostname_str.end());
  DC_LOG_DEBUG("[ConfigurationManager] Hostname (digits removed): %s", hostname_str.c_str());

  std::string lookup_file_suffix = std::string(DATACRUMBS_INSTALL_USER) + "-" + hostname_str;

  // Construct probe file name: probes-DATACRUMBS_INSTALL_USER-host.json
  std::string probe_file_name = "probes-" + lookup_file_suffix + ".json";
  this->probe_file_path = this->data_dir / probe_file_name;
  if (this->mpi_rank == 0)
    DC_LOG_DEBUG("[ConfigurationManager] Probe file path: %s",
                 this->probe_file_path.string().c_str());

  // Construct probe exclusion file name: probes-exclusion-DATACRUMBS_INSTALL_USER-host.json
  std::string probe_exclusion_file_name = "probes-exclusion-" + lookup_file_suffix + ".json";
  this->probe_exclusion_file_path = this->data_dir / probe_exclusion_file_name;
  if (this->mpi_rank == 0)
    DC_LOG_DEBUG("[ConfigurationManager] Probe exclusion file path: %s",
                 this->probe_exclusion_file_path.string().c_str());

  // Construct probe invalid file name: probes-invalid-DATACRUMBS_INSTALL_USER-host.json
  std::string probe_invalid_file_name = "probes-invalid-" + lookup_file_suffix + ".json";
  this->probe_invalid_file_path = this->data_dir / probe_invalid_file_name;
  if (this->mpi_rank == 0)
    DC_LOG_DEBUG("[ConfigurationManager] Probe invalid path: %s",
                 this->probe_invalid_file_path.string().c_str());

  // Construct categories file name: categories-DATACRUMBS_INSTALL_USER-host.json
  std::string categories_file_name = "categories-" + lookup_file_suffix + ".json";
  this->category_map_path = this->data_dir / categories_file_name;
  if (this->mpi_rank == 0)
    DC_LOG_DEBUG("[ConfigurationManager] Category map path: %s",
                 this->category_map_path.string().c_str());

  // Construct manual probe file name: manual-probes-DATACRUMBS_INSTALL_USER-host.json
  std::string manual_probe_file_name = "manual-probes-" + lookup_file_suffix + ".json";
  this->manual_probe_path = this->data_dir / manual_probe_file_name;
  if (this->mpi_rank == 0)
    DC_LOG_DEBUG("[ConfigurationManager] Manual probe path: %s",
                 this->manual_probe_path.string().c_str());

  // Load category_map from JSON file using json-c
  std::string category_json_path = category_map_path.string();
  if (!category_json_path.empty() && std::filesystem::exists(category_json_path)) {
    // Read file into string
    std::ifstream file(category_json_path);
    if (!file) {
      DC_LOG_ERROR("Failed to open category map file: %s for rank: %d", category_json_path.c_str(),
                   this->mpi_rank);
      throw std::invalid_argument("Failed to open category map file: " + category_json_path);
    }
    std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Parse JSON
    struct json_object* root = json_tokener_parse(json_str.c_str());
    if (!root) {
      DC_LOG_ERROR("Failed to parse JSON from %s for rank: %d", category_json_path.c_str(),
                   this->mpi_rank);
      throw std::invalid_argument("Failed to parse JSON from: " + category_json_path);
    }

    // Expecting a JSON object with event_id as keys
    json_object_object_foreach(root, key, val) {
      uint64_t event_id = std::stoull(key);
      const char* probe_name = nullptr;
      const char* function_name = nullptr;

      struct json_object* probe_obj = nullptr;
      struct json_object* func_obj = nullptr;

      if (json_object_object_get_ex(val, "probe_name", &probe_obj) &&
          json_object_object_get_ex(val, "function_name", &func_obj)) {
        probe_name = json_object_get_string(probe_obj);
        function_name = json_object_get_string(func_obj);
        category_map[event_id] =
            std::make_pair(probe_name ? probe_name : "", function_name ? function_name : "");
      }
    }
    json_object_put(root);
  } else {
    DC_LOG_WARN("[ConfigurationManager] Category map file does not exist: %s for rank: %d",
                category_json_path.c_str(), this->mpi_rank);
  }
}

/**
 * @brief Validates the loaded and derived configuration values.
 *
 * Checks for the presence of capture probes and the existence of required directories.
 * Throws exceptions if validation fails.
 */
void ConfigurationManager::validate_configurations() {
  if (this->capture_probes.empty()) {
    DC_LOG_ERROR(
        "[ConfigurationManager] No capture probes defined in the configuration for rank: %d.",
        this->mpi_rank);
    throw std::invalid_argument("At least one capture probe must be defined.");
  }
  if (this->data_dir.empty() || !std::filesystem::exists(this->data_dir)) {
    DC_LOG_ERROR("[ConfigurationManager] Data directory does not exist: %s for rank: %d.",
                 this->data_dir.string().c_str(), this->mpi_rank);
    throw std::runtime_error("Data directory does not exist: " + this->data_dir.string());
  }
  if (this->trace_log_dir.empty() ||
      !std::filesystem::exists(std::filesystem::path(this->trace_log_dir))) {
    DC_LOG_ERROR("[ConfigurationManager] Trace log directory does not exist: %s for rank: %d.",
                 this->trace_log_dir.string().c_str(), this->mpi_rank);
    throw std::runtime_error("Trace log directory does not exist: " +
                             std::filesystem::path(this->trace_log_dir).string());
  }
}

void ConfigurationManager::load_mpi_configurations() {
  if (this->disable_mpi) {
    this->mpi_rank = 0;
    this->mpi_size = 1;
    DC_LOG_DEBUG("[ConfigurationManager] MPI disabled, setting rank to 0 and size to 1.");
  } else {
    DC_LOG_DEBUG("[ConfigurationManager] MPI enabled, initializing rank and size.");
    int initialized;
    int status = MPI_Initialized(&initialized);
    if (status == MPI_SUCCESS && initialized == true) {
      MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
      MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    }
  }
}

}  // namespace datacrumbs
