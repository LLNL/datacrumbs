
/**
 * Internal headers
 */
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/utils.h>
/**
 * External dependencies
 */
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
namespace datacrumbs {

template <>
std::shared_ptr<datacrumbs::ConfigurationManager>
    datacrumbs::Singleton<datacrumbs::ConfigurationManager>::instance = nullptr;
template <>
bool datacrumbs::Singleton<datacrumbs::ConfigurationManager>::stop_creating_instances = false;

#define DC_YAML_MODE "mode"
#define DC_YAML_TRACE_LOG_DIR "trace_log_dir"
#define DC_YAML_DATA_DIR "data_dir"
#define DC_YAML_PROFILING_INTERVAL "profiling_interval"
#define DC_YAML_CAPTURE_PROBES "capture_probes"

/**
 * FIXME:
 */
#define DATACRUMBS_CONFIG_PATH "/home/haridev/datacrumbs/datacrumbs/configs/module"
#define DATACRUMBS_LOG_DIR "/home/haridev/datacrumbs/logs"
#define DATACRUMBS_DATA_DIR "/home/haridev/datacrumbs/data"

#include <iostream>

class ArgumentParser {
 public:
  std::string config_name;
  std::optional<std::string> mode;
  std::optional<std::string> trace_log_dir;
  std::optional<float> profiling_interval;
  std::optional<std::string> config_path;

  ArgumentParser(int argc, char** argv) {
    std::cout << "[ArgumentParser] Parsing command line arguments..." << std::endl;
    if (argc < 2) {
      throw std::invalid_argument("Configuration name is required as the first argument.");
    }
    config_name = argv[1];

    for (int i = 2; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--mode" && i + 1 < argc) {
        mode = argv[++i];
        std::cout << "[ArgumentParser] Mode set to: " << *mode << std::endl;
      } else if (arg == "--trace_log_dir" && i + 1 < argc) {
        trace_log_dir = argv[++i];
        std::cout << "[ArgumentParser] Trace log dir set to: " << *trace_log_dir << std::endl;
      } else if (arg == "--data_dir" && i + 1 < argc) {
        config_path = argv[++i];
        std::cout << "[ArgumentParser] Data directory set to: " << *config_path << std::endl;
      } else if (arg == "--profiling_interval" && i + 1 < argc) {
        profiling_interval = std::stof(argv[++i]);
        std::cout << "[ArgumentParser] Profiling interval set to: " << *profiling_interval
                  << std::endl;
      } else if (arg == "--config_path" && i + 1 < argc) {
        config_path = argv[++i];
        std::cout << "[ArgumentParser] Config path set to: " << *config_path << std::endl;
      } else if (arg == "--help" || arg == "-h") {
        std::cout << "Usage: " << argv[0] << " <config_name> [--mode <mode>] "
                  << "[--trace_file_path <path>] [--profiling_interval <seconds>] "
                  << "[--config_path <path>]\n";
        exit(0);
      } else {
        std::cerr << "[ArgumentParser] Unknown argument: " << arg << std::endl;
        throw std::invalid_argument("Unknown argument: " + arg);
      }
    }
  }
};

ConfigurationManager::ConfigurationManager(int argc, char** argv)
    : path(DATACRUMBS_CONFIG_PATH),
      name("default"),
      mode(Mode::PROFILER),
      trace_log_dir(DATACRUMBS_LOG_DIR),
      capture_probes(),
      profiling_interval(0.1f)  // Default profiling interval in seconds
{
  std::cout << "[ConfigurationManager] Initializing with arguments..." << std::endl;
  ArgumentParser parser(argc, argv);
  this->name = parser.config_name;
  YAML::Node config;
  std::filesystem::path config_path = this->path / (this->name + ".yaml");
  std::cout << "[ConfigurationManager] Loading configuration file: " << config_path << std::endl;
  try {
    config = YAML::LoadFile(config_path.string());
    std::cout << "[ConfigurationManager] Configuration file loaded successfully." << std::endl;
  } catch (const YAML::ParserException& e) {
    std::cerr << "[ConfigurationManager] Failed to parse configuration file: " << config_path
              << std::endl;
    throw std::runtime_error("Failed to parse configuration file: " + config_path.string());
  } catch (const YAML::BadFile& e) {
    std::cerr << "[ConfigurationManager] Failed to load configuration file: " << config_path
              << std::endl;
    throw std::runtime_error("Failed to load configuration file: " + config_path.string());
  }

  if (config != nullptr) {
    std::cout << "[ConfigurationManager] Parsing configuration YAML..." << std::endl;
    if (config[DC_YAML_MODE]) {
      convert(config[DC_YAML_MODE].as<std::string>(), this->mode);
      std::cout << "[ConfigurationManager] Mode set from config: "
                << config[DC_YAML_MODE].as<std::string>() << std::endl;
    }
    if (config[DC_YAML_TRACE_LOG_DIR]) {
      this->trace_log_dir = config[DC_YAML_TRACE_LOG_DIR].as<std::string>();
      std::cout << "[ConfigurationManager] Trace log dir set from config: " << this->trace_log_dir
                << std::endl;
    }
    if (config[DC_YAML_DATA_DIR]) {
      this->data_dir = config[DC_YAML_DATA_DIR].as<std::string>();
      std::cout << "[ConfigurationManager] Data directory set from config: " << this->data_dir
                << std::endl;
    } else {
      this->data_dir = DATACRUMBS_DATA_DIR;
      std::cout << "[ConfigurationManager] Data directory not specified, using default: "
                << this->data_dir << std::endl;
    }
    if (config[DC_YAML_PROFILING_INTERVAL]) {
      this->profiling_interval = config[DC_YAML_PROFILING_INTERVAL].as<float>();
      std::cout << "[ConfigurationManager] Profiling interval set from config: "
                << this->profiling_interval << std::endl;
    }
    if (config[DC_YAML_CAPTURE_PROBES]) {
      std::cout << "[ConfigurationManager] Parsing capture probes..." << std::endl;
      for (const auto& probe_node : config[DC_YAML_CAPTURE_PROBES]) {
        if (probe_node["type"]) {
          CaptureType type;
          convert(probe_node["type"].as<std::string>(), type);

          std::shared_ptr<CaptureProbe> probe;

          switch (type) {
            case CaptureType::HEADER: {
              auto header_probe = std::make_shared<HeaderCaptureProbe>();
              if (probe_node["file"]) {
                header_probe->file = probe_node["file"].as<std::string>();
                std::cout << "[ConfigurationManager] Added HEADER probe: " << header_probe->file
                          << std::endl;
              } else {
                std::cerr << "[ConfigurationManager] Header name missing for HEADER capture type."
                          << std::endl;
                throw std::invalid_argument("Header name is required for HEADER capture type.");
              }
              probe = header_probe;
              break;
            }
            case CaptureType::BINARY: {
              auto binary_probe = std::make_shared<BinaryCaptureProbe>();
              if (probe_node["file"]) {
                binary_probe->file = probe_node["file"].as<std::string>();
                std::cout << "[ConfigurationManager] Added BINARY probe: " << binary_probe->file
                          << std::endl;
              } else {
                std::cerr << "[ConfigurationManager] Binary path missing for BINARY capture type."
                          << std::endl;
                throw std::invalid_argument("Binary path is required for BINARY capture type.");
              }
              probe = binary_probe;
              break;
            }
            case CaptureType::KSYM: {
              probe = std::make_shared<KernelCaptureProbe>();
              std::cout << "[ConfigurationManager] Added KSYM probe." << std::endl;
              if (probe_node["regex"]) {
                probe->regex = probe_node["regex"].as<std::string>();
                std::cout << "[ConfigurationManager] KSYM probe regex set: " << probe->regex
                          << std::endl;
              } else {
                std::cerr << "[ConfigurationManager] Regex missing for KSYM capture type."
                          << std::endl;
                throw std::invalid_argument("Regex is required for KSYM capture type.");
              }
              break;
            }
            case CaptureType::USDT: {
              auto usdt_probe = std::make_shared<USDTCaptureProbe>();
              if (probe_node["binary_path"]) {
                usdt_probe->binary_path = probe_node["binary_path"].as<std::string>();
                std::cout << "[ConfigurationManager] Added USDT probe: " << usdt_probe->binary_path
                          << std::endl;
              } else {
                std::cerr << "[ConfigurationManager] Binary path missing for USDT capture type."
                          << std::endl;
                throw std::invalid_argument("Binary path is required for USDT capture type.");
              }
              if (probe_node["provider"]) {
                usdt_probe->provider = probe_node["provider"].as<std::string>();
                std::cout << "[ConfigurationManager] USDT provider set: " << usdt_probe->provider
                          << std::endl;
              } else {
                std::cerr << "[ConfigurationManager] Provider missing for USDT capture type."
                          << std::endl;
                throw std::invalid_argument("Provider is required for USDT capture type.");
              }
              probe = usdt_probe;
              break;
            }
            default:
              std::cerr << "[ConfigurationManager] Unknown CaptureType: "
                        << probe_node["type"].as<std::string>() << std::endl;
              throw std::invalid_argument("Unknown CaptureType in configuration: " +
                                          probe_node["type"].as<std::string>());
          }
          if (probe_node["probe"]) {
            auto probe_type_str = probe_node["probe"].as<std::string>();
            convert(probe_type_str, probe->probe_type);
            std::cout << "[ConfigurationManager] Probe type set: " << probe_type_str << std::endl;
          } else {
            std::cerr << "[ConfigurationManager] Probe type missing for capture type: "
                      << probe_node["type"].as<std::string>() << std::endl;
            throw std::invalid_argument("Probe type is required for capture type: " +
                                        probe_node["type"].as<std::string>());
          }
          if (probe_node["name"]) {
            probe->name = probe_node["name"].as<std::string>();
          } else {
            std::cerr << "[ConfigurationManager] Probe name missing for capture type: "
                      << probe_node["type"].as<std::string>() << std::endl;
            throw std::invalid_argument("Probe name is required for capture type: " +
                                        probe_node["type"].as<std::string>());
          }
          if (probe_node["regex"]) {
            probe->regex = probe_node["regex"].as<std::string>();
            std::cout << "[ConfigurationManager] Probe regex set: " << probe->regex << std::endl;
          } else {
            std::cout << "[ConfigurationManager] No regex provided for probe: " << probe->name
                      << std::endl;
          }
          if (probe) {
            this->capture_probes.push_back(probe);
          }
        }
      }
    }

    if (parser.config_path) {
      this->path = *parser.config_path;
      std::cout << "[ConfigurationManager] Config path overridden by argument: " << this->path
                << std::endl;
    }
    if (parser.mode) {
      convert(*parser.mode, this->mode);
      std::cout << "[ConfigurationManager] Mode overridden by argument: " << *parser.mode
                << std::endl;
    }
    if (parser.trace_log_dir) {
      this->trace_log_dir = *parser.trace_log_dir;
      std::cout << "[ConfigurationManager] Trace log dir overridden by argument: "
                << *parser.trace_log_dir << std::endl;
    }
    if (parser.profiling_interval) {
      this->profiling_interval = *parser.profiling_interval;
      std::cout << "[ConfigurationManager] Profiling interval overridden by argument: "
                << *parser.profiling_interval << std::endl;
    }
  }
  derive_configurations();
  validate_configurations();
  std::cout << "[ConfigurationManager] Initialization complete." << std::endl;
};

void ConfigurationManager::derive_configurations() {
  std::string user = std::getenv("USER") ? std::getenv("USER") : "unknown";
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    std::strncpy(hostname, "unknownhost", sizeof(hostname));
    hostname[sizeof(hostname) - 1] = '\0';
  }
  pid_t pid = getpid();
  auto now = std::chrono::system_clock::now();
  auto timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

  // Simple encoding: base64 of hostname + pid + timestamp
  std::stringstream ss;
  ss << hostname << "_" << pid << "_" << timestamp;
  std::string raw = ss.str();
  auto encoded =
      datacrumbs::utils::base64_encode(std::vector<unsigned char>(raw.begin(), raw.end()));
  std::string trace_file_name = "trace_" + user + "_" + encoded + ".pfw";
  this->trace_file_path = this->trace_log_dir / trace_file_name;

  std::string hostname_str(hostname);
  // Remove digits from hostname
  hostname_str.erase(std::remove_if(hostname_str.begin(), hostname_str.end(), ::isdigit),
                     hostname_str.end());

  // Construct probe file name: probes-user-host.json
  std::string probe_file_name = "probes-" + user + "-" + hostname_str + ".json";
  this->probe_file_path = this->data_dir / probe_file_name;
  // Construct categories file name: categories-user-host.json
  std::string categories_file_name = "categories-" + user + "-" + hostname_str + ".json";
  this->category_map_path = this->data_dir / categories_file_name;
}
void ConfigurationManager::validate_configurations() {
  if (this->capture_probes.empty()) {
    std::cerr << "[ConfigurationManager] No capture probes defined in the configuration."
              << std::endl;
    throw std::invalid_argument("At least one capture probe must be defined.");
  }
  if (!std::filesystem::exists(this->data_dir)) {
    std::cerr << "[ConfigurationManager] Data directory does not exist: " << this->data_dir
              << std::endl;
    throw std::runtime_error("Data directory does not exist: " + this->data_dir.string());
  }
  if (!std::filesystem::exists(std::filesystem::path(this->trace_log_dir))) {
    std::cerr << "[ConfigurationManager] Trace log directory does not exist: "
              << this->trace_log_dir << std::endl;
    throw std::runtime_error("Trace log directory does not exist: " +
                             std::filesystem::path(this->trace_log_dir).string());
  }
}

}  // namespace datacrumbs