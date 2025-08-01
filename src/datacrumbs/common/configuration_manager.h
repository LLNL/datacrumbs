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
  std::filesystem::path path;   // Path to the configuration file
  std::string name;             // Name of the configuration file
  Mode mode;                    // Mode of operation (PROFILER or TRACER)
  std::string trace_file_path;  // Path to the trace file
  std::vector<std::shared_ptr<CaptureProbe>> capture_probes;  // List of capture probes

  // Profiling specific configurations
  float profiling_interval;  // Interval for profiling in seconds

  ConfigurationManager(int argc, char** argv);
};

}  // namespace datacrumbs
