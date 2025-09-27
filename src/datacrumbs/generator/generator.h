#pragma once

// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
// Include necessary headers for configuration, data structures, enumerations, and probe generators
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/constants.h>
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/enumerations.h>
#include <datacrumbs/common/logging.h>  // Added for logging macros
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/utils.h>
#include <datacrumbs/generator/mechanisms/kprobe_generator.h>
#include <datacrumbs/generator/mechanisms/syscall_generator.h>
#include <datacrumbs/generator/mechanisms/uprobe_generator.h>
#include <datacrumbs/generator/mechanisms/usdt_generator.h>

// dependency headers
#include <json-c/json.h>
// std headers
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace datacrumbs {

/**
 * @brief ProbeGenerator is responsible for generating probes based on configuration and input
 * files.
 */
class ProbeGenerator {
 public:
  /**
   * @brief Constructor for ProbeGenerator.
   * @param argc Argument count from main.
   * @param argv Argument vector from main.
   */
  ProbeGenerator(int argc, char** argv);

  /**
   * @brief Main entry point to run the probe generation process.
   * This method reads the configuration, processes the probes file,
   * generates probe files, and writes the category map.
   * @return Total number of probes generated.
   */
  int run();

 private:
  // Shared pointer to the configuration manager
  std::shared_ptr<ConfigurationManager> configManager_;

  /**
   * @brief Helper function to extract a string value from a JSON object by key.
   * @param obj JSON object to extract from.
   * @param key Key to look up in the JSON object.
   * @return Pointer to the string value, or nullptr if not found.
   */
  static const char* get_string_from_json(struct json_object* obj, const char* key) {
    struct json_object* val = nullptr;
    if (json_object_object_get_ex(obj, key, &val) && json_object_is_type(val, json_type_string)) {
      return json_object_get_string(val);
    }
    return nullptr;
  }

  /**
   * @brief Writes the category map to the output file.
   */
  void writeCategoryMap();

  int update_event(const std::string& probe_name, const std::string& function_name, int event_id) {
    struct json_object* info = json_object_new_object();
    json_object_object_add(info, "probe_name", json_object_new_string(probe_name.c_str()));
    json_object_object_add(info, "function_name", json_object_new_string(function_name.c_str()));
    categoryMap_[event_id] = info;
    return 0;
  }

  // Path to the probes file
  std::string probesFile_;
  // Path to the category map file
  std::string categoryMapFile_;
  // Counter for event IDs
  int eventIdCounter_;
  // Map from event ID to JSON object representing the category
  std::unordered_map<int, struct json_object*> categoryMap_;
};

}  // namespace datacrumbs
