#pragma once

/**
 * @file configuration_manager.h
 * @brief Internal header for the ConfigurationManager class.
 *
 * This file defines the ConfigurationManager class, which is responsible for
 * managing, validating, and deriving configuration settings for the DataCrumbs library.
 */
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/enumerations.h>
#include <datacrumbs/common/logging.h>

// std headers
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace datacrumbs {

/**
 * @class ConfigurationManager
 * @brief Manages configuration settings for the DataCrumbs library.
 *
 * The ConfigurationManager class handles loading, validating, and deriving
 * configuration settings based on command-line arguments. It ensures that all
 * required configurations are set and valid before the library is used.
 */
class ConfigurationManager {
 public:
  // Path to the configuration file provided by the user
  std::filesystem::path path;

  // Directory for data storage
  std::filesystem::path data_dir;

  // Name of the configuration file
  std::string name;

  // Directory where trace logs will be stored
  std::filesystem::path trace_log_dir;

  // List of capture probes to be used in the session
  std::vector<std::shared_ptr<CaptureProbe>> capture_probes;

  // User associated with the configuration
  std::string user;

  std::string inclusion_path;  // Path to the inclusion file

  std::string log_dir;  // Directory for log files

  // Derived configuration: path to the trace file
  std::filesystem::path trace_file_path;

  // Derived configuration: path to the probe file
  std::filesystem::path probe_file_path;

  // Derived configuration: path to the probe exclusion file
  std::filesystem::path probe_exclusion_file_path;

  // Derived configuration: path to the probe invalid file
  std::filesystem::path probe_invalid_file_path;

  // Derived configuration: path to the category map file
  std::filesystem::path category_map_path;

  // Derived configuration: path to the manual probe file
  std::filesystem::path manual_probe_path;

  // Derived configuration: category map for event IDs
  std::unordered_map<uint64_t, std::pair<std::string, std::string>> category_map;

  // Derived configuration: current hostname
  std::string hostname;

  /**
   * @brief Constructor that initializes the ConfigurationManager with command-line arguments.
   *
   * Parses the command-line arguments to set up the configuration, derives necessary
   * configurations, and validates them. If any required configuration is missing or invalid,
   * logs an error and exits the program.
   *
   * @param argc Number of command-line arguments
   * @param argv Array of command-line argument strings
   */
  ConfigurationManager(int argc, char** argv, bool print = true, int start_index = 1);

  ConfigurationManager() {
    // Default constructor for internal use
  }

  // For debugging: prints all configuration values to the log
  void print_configurations();

 private:
  /**
   * @brief Derives configurations based on the provided command-line arguments.
   *
   * Sets up paths and other configurations based on the mode of operation.
   */
  void derive_configurations();

  /**
   * @brief Validates the derived configurations.
   *
   * Checks if all required configurations are set and valid. If any configuration is invalid,
   * logs an error and exits the program. This ensures correct operation of the DataCrumbs library.
   */
  void validate_configurations();

  // Loads the category map from the specified JSON file
  void load_category_map();
};

class ArgumentParser {
 public:
  std::string config_name;                          ///< Name of the configuration to load
  std::optional<std::string> trace_log_dir;         ///< Optional trace log directory
  std::optional<std::string> config_path;           ///< Optional configuration file path
  std::optional<std::string> data_dir;              ///< Optional data directory
  std::optional<std::string> user;                  ///< Optional user argument
  std::optional<uint64_t> skip_event_threshold_us;  ///< Optional skip event threshold
  std::optional<std::string> inclusion_path;        ///< Optional inclusion path
  std::optional<std::string> log_dir;               ///< Optional log directory

  /**
   * @brief Constructor that parses command-line arguments.
   * @param argc Number of command-line arguments
   * @param argv Array of command-line argument strings
   * @throws std::invalid_argument if required arguments are missing or unknown arguments are found
   */
  ArgumentParser(int argc, char** argv, int start_index = 1);
};

}  // namespace datacrumbs
