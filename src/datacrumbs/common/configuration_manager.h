#pragma once

/**
 * Internal header for the ConfigurationManager class.
 */
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/enumerations.h>
/**
 * External dependencies.
 */
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace datacrumbs {

class ConfigurationManager {
 public:
  std::filesystem::path path;           // Path to the configuration file
  std::filesystem::path data_dir;       // Directory for data storage
  std::string name;                     // Name of the configuration file
  Mode mode;                            // Mode of operation (PROFILER or TRACER)
  std::filesystem::path trace_log_dir;  // Path to the trace file
  std::vector<std::shared_ptr<CaptureProbe>> capture_probes;  // List of capture probes

  // Profiling specific configurations
  float profiling_interval;  // Interval for profiling in seconds

  // Derived configurations
  std::filesystem::path trace_file_path;            // Path to the trace file
  std::filesystem::path probe_file_path;            // Path to the probe file
  std::filesystem::path probe_exclusion_file_path;  // Path to the probe file
  std::filesystem::path category_map_path;          // Path to the category map file

  ConfigurationManager(int argc, char** argv);

 private:
  void derive_configurations();
  void validate_configurations();
};

}  // namespace datacrumbs
