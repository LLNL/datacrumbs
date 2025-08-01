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
  std::vector<Probe> extractProbes();

  std::vector<Probe> writeProbesToJson(const std::string& filename) {
    auto probes = extractProbes();
    json_object* jarray = json_object_new_array();

    for (const auto& probe : probes) {
      json_object* jprobe = probe.toJson();
      json_object_array_add(jarray, jprobe);
    }

    const char* json_str = json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PRETTY);

    std::ofstream ofs(filename);
    if (ofs.is_open()) {
      ofs << json_str;
      ofs.close();
    } else {
      std::cerr << "Failed to open file: " << filename << std::endl;
    }

    json_object_put(jarray);  // free memory
    return probes;
  }

 private:
  std::shared_ptr<ConfigurationManager> configManager_;
};
}  // namespace datacrumbs
