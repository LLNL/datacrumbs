#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/constants.h>
#include <datacrumbs/common/logging.h>  // Use custom logging macros
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/utils.h>
#include <datacrumbs/explorer/mechanism/elf_capture.h>
#include <datacrumbs/explorer/mechanism/header_capture.h>
#include <datacrumbs/explorer/mechanism/ksym_capture.h>
#include <datacrumbs/explorer/mechanism/usdt_functions.h>

// dependency libraries
#include <json-c/json.h>

// std headers
#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace datacrumbs {

// ProbeExplorer class is responsible for extracting and writing probes
class ProbeExplorer {
 public:
  // Constructor: Initializes ProbeExplorer with command-line arguments
  ProbeExplorer(int argc, char** argv);
  // Extracts exclusion mappings and check for invalid entries
  // Returns a map of probe names to sets of function names to be excluded
  std::unordered_map<std::string, std::unordered_set<std::string>> Extract_Exclusions();
  // Extracts probes from a given data source (dummy implementation)
  // Returns a vector of shared pointers to Probe objects
  std::vector<std::shared_ptr<Probe>> extractProbes();
  // Creates an exclusion file from the provided probes if the file does not exist
  void create_exclusion_file(std::vector<std::shared_ptr<Probe>> probes);
  // Writes extracted probes to a JSON file
  // Returns a vector of shared pointers to Probe objects
  std::vector<std::shared_ptr<Probe>> writeProbesToJson();

  bool has_invalid_probes_ = false;  // Flag to indicate if any invalid probes were found

 private:
  // Configuration manager instance for managing configuration settings
  std::shared_ptr<ConfigurationManager> configManager_;
};

}  // namespace datacrumbs
