#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// std headers
#include <any>
#include <string>
#include <unordered_map>
#include <vector>

struct CapturedArgumentValue {
  std::string c_type;
  bool is_pointer = false;
  unsigned long long raw_value = 0;
  unsigned int data_status = 0;
  std::vector<unsigned char> bytes;
};

typedef std::unordered_map<std::string, std::any> DataCrumbsArgs;
