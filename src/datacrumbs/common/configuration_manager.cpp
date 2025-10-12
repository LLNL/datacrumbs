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
#include <datacrumbs/common/logging.h>  // <-- Added logging header
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/utils.h>
/**
 * External headers
 */
#include <yaml-cpp/yaml.h>

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

ArgumentParser::ArgumentParser(int argc, char** argv, int start_index) {
  DC_LOG_TRACE("[ArgumentParser] Parsing command line arguments...");
  if (argc < 2) {
    throw std::invalid_argument("Configuration name is required as the first argument.");
  }
  config_name = argv[start_index];

  for (int i = start_index + 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--trace_log_dir" && i + 1 < argc) {
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
    } else if (arg == "--help" || arg == "-h") {
      DC_LOG_PRINT(
          "Usage: %s <config_name> [--trace_log_dir <path>] "
          "[--config_path <path>] [--user <user>] [--data_dir "
          "<path>] [--inclusion_path <path>] [--log_dir <path>]",
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
ConfigurationManager::ConfigurationManager(int argc, char** argv, bool print, int start_index)
    : path(DATACRUMBS_CONFIG_PATH),
      name("default"),
      trace_log_dir(DATACRUMBS_LOG_DIR),
      capture_probes(),
      user("datacrumbs") {
  DC_LOG_TRACE("[ConfigurationManager] Initializing with arguments...");
  ArgumentParser parser(argc, argv, start_index);
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
    // Parse capture probes from YAML
    if (config[DC_YAML_CAPTURE_PROBES]) {
      DC_LOG_TRACE("[ConfigurationManager] Parsing capture probes...");
      for (const auto& probe_node : config[DC_YAML_CAPTURE_PROBES]) {
        if (probe_node["type"]) {
          CaptureType type;
          convert(probe_node["type"].as<std::string>(), type);

          std::shared_ptr<CaptureProbe> probe;

          // Handle each capture probe type
          switch (type) {
            case CaptureType::HEADER: {
              auto header_probe = std::make_shared<HeaderCaptureProbe>();
              if (probe_node["file"]) {
                header_probe->file = probe_node["file"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] Added HEADER probe: %s",
                             header_probe->file.c_str());
              } else {
                DC_LOG_ERROR("[ConfigurationManager] Header name missing for HEADER capture type.");
                throw std::invalid_argument("Header name is required for HEADER capture type.");
              }
              probe = header_probe;
              break;
            }
            case CaptureType::BINARY: {
              auto binary_probe = std::make_shared<BinaryCaptureProbe>();
              if (probe_node["file"]) {
                binary_probe->file = probe_node["file"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] Added BINARY probe: %s",
                             binary_probe->file.c_str());
              } else {
                DC_LOG_ERROR("[ConfigurationManager] Binary path missing for BINARY capture type.");
                throw std::invalid_argument("Binary path is required for BINARY capture type.");
              }
              if (probe_node["include_offsets"]) {
                binary_probe->include_offsets = probe_node["include_offsets"].as<bool>();
                DC_LOG_DEBUG("[ConfigurationManager] BINARY include_offsets set to: %s",
                             binary_probe->include_offsets ? "true" : "false");
              } else {
                DC_LOG_DEBUG(
                    "[ConfigurationManager] No include_offsets provided for BINARY, using default: "
                    "false");
                binary_probe->include_offsets = false;  // Default value
              }
              probe = binary_probe;
              break;
            }
            case CaptureType::KSYM: {
              probe = std::make_shared<KernelCaptureProbe>();
              DC_LOG_DEBUG("[ConfigurationManager] Added KSYM probe.");
              if (probe_node["regex"]) {
                probe->regex = probe_node["regex"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] KSYM probe regex set: %s",
                             probe->regex.c_str());
              } else {
                DC_LOG_ERROR("[ConfigurationManager] Regex missing for KSYM capture type.");
                throw std::invalid_argument("Regex is required for KSYM capture type.");
              }
              break;
            }
            case CaptureType::USDT: {
              auto usdt_probe = std::make_shared<USDTCaptureProbe>();
              if (probe_node["binary_path"]) {
                usdt_probe->binary_path = probe_node["binary_path"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] Added USDT probe: %s",
                             usdt_probe->binary_path.c_str());
              } else {
                DC_LOG_ERROR("[ConfigurationManager] Binary path missing for USDT capture type.");
                throw std::invalid_argument("Binary path is required for USDT capture type.");
              }
              if (probe_node["provider"]) {
                usdt_probe->provider = probe_node["provider"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] USDT provider set: %s",
                             usdt_probe->provider.c_str());
              } else {
                DC_LOG_ERROR("[ConfigurationManager] Provider missing for USDT capture type.");
                throw std::invalid_argument("Provider is required for USDT capture type.");
              }
              probe = usdt_probe;
              break;
            }
            case CaptureType::CUSTOM: {
              auto custom_probe = std::make_shared<CustomCaptureProbe>();
              if (probe_node["file"]) {
                custom_probe->bpf_file = probe_node["file"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] Added CUSTOM probe: %s",
                             custom_probe->bpf_file.c_str());
              } else {
                DC_LOG_ERROR("[ConfigurationManager] BPF file missing for CUSTOM capture type.");
                throw std::invalid_argument("BPF file is required for CUSTOM capture type.");
              }
              if (probe_node["probes"]) {
                custom_probe->probe_file = probe_node["probes"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] Custom probe file set: %s",
                             custom_probe->probe_file.c_str());
              } else {
                DC_LOG_ERROR("[ConfigurationManager] Probe file missing for CUSTOM capture type.");
                throw std::invalid_argument("Probe file is required for CUSTOM capture type.");
              }
              if (probe_node["start_event_id"]) {
                custom_probe->start_event_id = probe_node["start_event_id"].as<uint64_t>();
                DC_LOG_DEBUG("[ConfigurationManager] Custom start event ID set: %lu",
                             custom_probe->start_event_id);
              } else {
                DC_LOG_DEBUG(
                    "[ConfigurationManager] No start event ID provided, using default: %lu",
                    custom_probe->start_event_id);
              }
              if (probe_node["process_header"]) {
                custom_probe->process_header = probe_node["process_header"].as<std::string>();
                DC_LOG_DEBUG("[ConfigurationManager] Custom process header set: %s",
                             custom_probe->process_header.c_str());
              } else {
                DC_LOG_DEBUG("[ConfigurationManager] No process header provided, using default.");
              }
              if (probe_node["event_type"]) {
                custom_probe->event_type = probe_node["event_type"].as<uint64_t>();
                DC_LOG_DEBUG("[ConfigurationManager] Custom event type set: %lu",
                             custom_probe->event_type);
              } else {
                DC_LOG_DEBUG("[ConfigurationManager] No event type provided, using default: 1");
                custom_probe->event_type = 1;  // Default event type
              }
              probe = custom_probe;
              break;
            }
            default:
              DC_LOG_ERROR("[ConfigurationManager] Unknown CaptureType: %s",
                           probe_node["type"].as<std::string>().c_str());
              throw std::invalid_argument("Unknown CaptureType in configuration: " +
                                          probe_node["type"].as<std::string>());
          }
          // Parse probe type
          if (probe_node["probe"]) {
            auto probe_type_str = probe_node["probe"].as<std::string>();
            convert(probe_type_str, probe->probe_type);
            DC_LOG_DEBUG("[ConfigurationManager] Probe type set: %s", probe_type_str.c_str());
          } else {
            DC_LOG_ERROR("[ConfigurationManager] Probe type missing for capture type: %s",
                         probe_node["type"].as<std::string>().c_str());
            throw std::invalid_argument("Probe type is required for capture type: " +
                                        probe_node["type"].as<std::string>());
          }
          // Parse probe name
          if (probe_node["name"]) {
            probe->name = probe_node["name"].as<std::string>();
          } else {
            DC_LOG_ERROR("[ConfigurationManager] Probe name missing for capture type: %s",
                         probe_node["type"].as<std::string>().c_str());
            throw std::invalid_argument("Probe name is required for capture type: " +
                                        probe_node["type"].as<std::string>());
          }
          // Parse optional regex
          if (probe_node["regex"]) {
            probe->regex = probe_node["regex"].as<std::string>();
            DC_LOG_DEBUG("[ConfigurationManager] Probe regex set: %s", probe->regex.c_str());
          } else {
            DC_LOG_TRACE("[ConfigurationManager] No regex provided for probe: %s",
                         probe->name.c_str());
          }
          // Add probe to the list
          if (probe) {
            this->capture_probes.push_back(probe);
          }
        }
      }
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
  DC_LOG_INFO("  User: %s", this->user.c_str());
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

  pid_t pid = getpid();
  DC_LOG_DEBUG("[ConfigurationManager] Process ID: %d", pid);

  // Use this->hostname (std::string) instead of local char array
  std::string hostname;
  char hostname_buf[256] = {0};
  if (gethostname(hostname_buf, sizeof(hostname_buf) - 1) != 0) {
    DC_LOG_ERROR("[ConfigurationManager] Failed to get hostname.");
    throw std::runtime_error("Failed to get hostname.");
  }
  hostname = hostname_buf;
  this->hostname = hostname;
  DC_LOG_DEBUG("[ConfigurationManager] Hostname: %s", this->hostname.c_str());

  auto now = std::chrono::system_clock::now();
  auto timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  DC_LOG_DEBUG("[ConfigurationManager] Timestamp: %lld", static_cast<long long>(timestamp));

  // Simple encoding: base64 of hostname + pid + timestamp
  std::stringstream ss;
  ss << this->name << "_" << pid << "_" << timestamp;
  std::string raw = ss.str();
  auto encoded =
      datacrumbs::utils::base64_encode(std::vector<unsigned char>(raw.begin(), raw.end()));
  std::string trace_file_name = "trace_" + user + "_" + encoded + ".pfw.gz";
  this->trace_file_path = this->trace_log_dir / trace_file_name;
  DC_LOG_DEBUG("[ConfigurationManager] Trace file path: %s",
               this->trace_file_path.string().c_str());

  std::string hostname_str(this->name);
  // Remove digits from hostname for file naming
  hostname_str.erase(std::remove_if(hostname_str.begin(), hostname_str.end(), ::isdigit),
                     hostname_str.end());
  DC_LOG_DEBUG("[ConfigurationManager] Hostname (digits removed): %s", hostname_str.c_str());

  // Construct probe file name: probes-user-host.json
  std::string probe_file_name = "probes-" + user + "-" + hostname_str + ".json";
  this->probe_file_path = this->data_dir / probe_file_name;
  DC_LOG_DEBUG("[ConfigurationManager] Probe file path: %s",
               this->probe_file_path.string().c_str());

  // Construct probe exclusion file name: probes-exclusion-user-host.json
  std::string probe_exclusion_file_name = "probes-exclusion-" + user + "-" + hostname_str + ".json";
  this->probe_exclusion_file_path = this->data_dir / probe_exclusion_file_name;
  DC_LOG_DEBUG("[ConfigurationManager] Probe exclusion file path: %s",
               this->probe_exclusion_file_path.string().c_str());

  // Construct probe invalid file name: probes-invalid-user-host.json
  std::string probe_invalid_file_name = "probes-invalid-" + user + "-" + hostname_str + ".json";
  this->probe_invalid_file_path = this->data_dir / probe_invalid_file_name;
  DC_LOG_DEBUG("[ConfigurationManager] Probe invalid path: %s",
               this->probe_invalid_file_path.string().c_str());

  // Construct categories file name: categories-user-host.json
  std::string categories_file_name = "categories-" + user + "-" + hostname_str + ".json";
  this->category_map_path = this->data_dir / categories_file_name;
  DC_LOG_DEBUG("[ConfigurationManager] Category map path: %s",
               this->category_map_path.string().c_str());

  // Construct manual probe file name: manual-probes-user-host.json
  std::string manual_probe_file_name = "manual-probes-" + user + "-" + hostname_str + ".json";
  this->manual_probe_path = this->data_dir / manual_probe_file_name;
  DC_LOG_DEBUG("[ConfigurationManager] Manual probe path: %s",
               this->manual_probe_path.string().c_str());

  // Load category_map from JSON file using json-c
  std::string category_json_path = category_map_path.string();
  if (!category_json_path.empty() && std::filesystem::exists(category_json_path)) {
    // Read file into string
    std::ifstream file(category_json_path);
    if (!file) {
      DC_LOG_ERROR("Failed to open category map file: %s", category_json_path.c_str());
      throw std::invalid_argument("Failed to open category map file: " + category_json_path);
    }
    std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Parse JSON
    struct json_object* root = json_tokener_parse(json_str.c_str());
    if (!root) {
      DC_LOG_ERROR("Failed to parse JSON from %s", category_json_path.c_str());
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
    DC_LOG_WARN("[ConfigurationManager] Category map file does not exist: %s",
                category_json_path.c_str());
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
    DC_LOG_ERROR("[ConfigurationManager] No capture probes defined in the configuration.");
    throw std::invalid_argument("At least one capture probe must be defined.");
  }
  if (this->data_dir.empty() || !std::filesystem::exists(this->data_dir)) {
    DC_LOG_ERROR("[ConfigurationManager] Data directory does not exist: %s",
                 this->data_dir.string().c_str());
    throw std::runtime_error("Data directory does not exist: " + this->data_dir.string());
  }
  if (this->trace_log_dir.empty() ||
      !std::filesystem::exists(std::filesystem::path(this->trace_log_dir))) {
    DC_LOG_ERROR("[ConfigurationManager] Trace log directory does not exist: %s",
                 this->trace_log_dir.string().c_str());
    throw std::runtime_error("Trace log directory does not exist: " +
                             std::filesystem::path(this->trace_log_dir).string());
  }
}

}  // namespace datacrumbs
