#pragma once

#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/logging.h>  // Use custom logging macros
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/explorer/mechanism/elf_capture.h>
#include <datacrumbs/explorer/mechanism/header_capture.h>
#include <json-c/json.h>

#include <fstream>
#include <string>
#include <vector>

namespace datacrumbs {

// ProbeExplorer class is responsible for extracting and writing probes
class ProbeExplorer {
 public:
  // Constructor: Initializes ProbeExplorer with command-line arguments
  ProbeExplorer(int argc, char** argv);

  // Extracts probes from a given data source (dummy implementation)
  // Returns a vector of shared pointers to Probe objects
  std::vector<std::shared_ptr<Probe>> extractProbes();

  // Writes extracted probes to a JSON file
  // Returns a vector of shared pointers to Probe objects
  std::vector<std::shared_ptr<Probe>> writeProbesToJson();

 private:
  // Configuration manager instance for managing configuration settings
  std::shared_ptr<ConfigurationManager> configManager_;
};

}  // namespace datacrumbs
