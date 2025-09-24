#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// std headers
#include <any>
#include <string>
#include <unordered_map>

typedef std::unordered_map<std::string, std::any> DataCrumbsArgs;
#define NORMAL_EVENT 'X'
#define COUNTER_EVENT 'C'
#define METADATA_EVENT 'M'