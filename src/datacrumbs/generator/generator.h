#pragma once

#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/enumerations.h>
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/generator/mechanisms/kprobe_generator.h>
#include <datacrumbs/generator/mechanisms/syscall_generator.h>
#include <datacrumbs/generator/mechanisms/uprobe_generator.h>
#include <datacrumbs/generator/mechanisms/usdt_generator.h>
#include <json-c/json.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

namespace datacrumbs {
class ProbeGenerator {
 public:
  ProbeGenerator(int argc, char** argv);

  void run();

 private:
  std::shared_ptr<ConfigurationManager> configManager_;
  static const char* get_string_from_json(struct json_object* obj, const char* key) {
    struct json_object* val = nullptr;
    if (json_object_object_get_ex(obj, key, &val) && json_object_is_type(val, json_type_string)) {
      return json_object_get_string(val);
    }
    return nullptr;
  }

  void writeCategoryMap();

  std::string probesFile_;
  std::string categoryMapFile_;
  int eventIdCounter_;
  std::unordered_map<int, struct json_object*> categoryMap_;
};
}  // namespace datacrumbs
