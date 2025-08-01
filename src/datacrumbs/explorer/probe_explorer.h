#pragma once

#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/explorer/mechanism/elf_capture.h>
#include <datacrumbs/explorer/mechanism/header_capture.h>
#include <json-c/json.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace datacrumbs {
// ProbeExplorer class definition
class ProbeExplorer {
 public:
  ProbeExplorer(int argc, char** argv);

  // Extracts probes from a given data source (dummy implementation)
  std::vector<std::shared_ptr<Probe>> extractProbes();

  std::vector<std::shared_ptr<Probe>> writeProbesToJson();

 private:
  std::shared_ptr<ConfigurationManager> configManager_;
};
}  // namespace datacrumbs
