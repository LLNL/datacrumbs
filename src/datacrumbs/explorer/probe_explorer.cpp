
#include <datacrumbs/explorer/probe_explorer.h>

namespace datacrumbs {

// Constructor for ProbeExplorer, initializes the configuration manager singleton
ProbeExplorer::ProbeExplorer(int argc, char** argv) {
  DC_LOG_TRACE("ProbeExplorer::ProbeExplorer - start");
  configManager_ = datacrumbs::Singleton<ConfigurationManager>::get_instance(argc, argv);
  has_invalid_probes_ = false;
  DC_LOG_TRACE("ProbeExplorer::ProbeExplorer - end");
}
std::unordered_map<std::string, std::unordered_set<std::string>>
ProbeExplorer::Extract_Exclusions() {
  DC_LOG_TRACE("ProbeExplorer::validate_exclusion_file - start");
  std::unordered_map<std::string, std::unordered_set<std::string>> exclusionMap;
  if (!configManager_->probe_exclusion_file_path.empty() &&
      std::filesystem::exists(configManager_->probe_exclusion_file_path)) {
    std::ifstream ifs(configManager_->probe_exclusion_file_path);
    if (ifs.is_open()) {
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      json_object* jobj = json_tokener_parse(content.c_str());
      if (jobj && json_object_get_type(jobj) == json_type_array) {
        int arr_len = json_object_array_length(jobj);
        for (int i = 0; i < arr_len; ++i) {
          json_object* probe_obj = json_object_array_get_idx(jobj, i);
          if (!probe_obj) {
            DC_LOG_WARN("the %dth element of exclusion file is missing (null pointer returned).",
                        i);
            continue;
          }
          if (json_object_get_type(probe_obj) == json_type_null) {
            DC_LOG_WARN("exclusion file contains explicit JSON null at %dth element.", i);
            continue;
          }
          json_object* name_obj = nullptr;
          json_object* funcs_obj = nullptr;
          if (json_object_object_get_ex(probe_obj, "name", &name_obj) &&
              json_object_object_get_ex(probe_obj, "functions", &funcs_obj) &&
              json_object_get_type(name_obj) == json_type_string &&
              json_object_get_type(funcs_obj) == json_type_array) {
            std::string probe_name = json_object_get_string(name_obj);
            std::unordered_set<std::string> func_set;
            int func_len = json_object_array_length(funcs_obj);
            for (int j = 0; j < func_len; ++j) {
              json_object* func_obj = json_object_array_get_idx(funcs_obj, j);

              if (func_obj && json_object_get_type(func_obj) == json_type_string) {
                // check the function name
                std::string func_name = json_object_get_string(func_obj);
                if (func_name.find('/') != std::string::npos ||
                    func_name.find('\\') != std::string::npos ||
                    func_name.find(' ') != std::string::npos) {
                  DC_LOG_WARN(
                      "Exclusion file contains invalid function name '%s' for probe '%s'. Skipping "
                      "this function.",
                      func_name.c_str(), probe_name.c_str());
                  continue;
                }
                func_set.insert(json_object_get_string(func_obj));
              }
            }
            exclusionMap[probe_name] = std::move(func_set);
          } else {
            DC_LOG_WARN(
                "Exclusion file entry at index %d is missing 'name' or 'functions' field, or they "
                "are of incorrect type.",
                i);
          }
        }
      } else {
        DC_LOG_WARN("Exclusion file is not a valid JSON array.");
      }
      if (jobj) json_object_put(jobj);
    } else {
      DC_LOG_ERROR("Failed to open exclusion probes file: %s",
                   configManager_->probe_exclusion_file_path.string().c_str());
    }
  }
  return exclusionMap;

  DC_LOG_TRACE("ProbeExplorer::validate_exclusion_file - end");
}
// Extracts probes based on configuration and exclusion file
std::vector<std::shared_ptr<Probe>> ProbeExplorer::extractProbes() {
  DC_LOG_TRACE("ProbeExplorer::extractProbes - start");
  auto exclusionMap = Extract_Exclusions();

  // Log the contents of the exclusion map for debugging
  DC_LOG_DEBUG("Exclusion Map Contents:");
  for (const auto& [probe_name, func_set] : exclusionMap) {
    DC_LOG_DEBUG("Probe: %s", probe_name.c_str());
    for (const auto& func : func_set) {
      DC_LOG_DEBUG("  Excluded Function: %s", func.c_str());
    }
  }

  // Load additional invalid probes from file if specified
  if (!configManager_->probe_invalid_file_path.empty() &&
      std::filesystem::exists(configManager_->probe_invalid_file_path)) {
    std::ifstream ifs(configManager_->probe_invalid_file_path);
    if (ifs.is_open()) {
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      json_object* jobj = json_tokener_parse(content.c_str());
      if (jobj && json_object_get_type(jobj) == json_type_array) {
        int arr_len = json_object_array_length(jobj);
        for (int i = 0; i < arr_len; ++i) {
          json_object* probe_obj = json_object_array_get_idx(jobj, i);
          if (!probe_obj) continue;
          json_object* name_obj = nullptr;
          json_object* funcs_obj = nullptr;
          if (json_object_object_get_ex(probe_obj, "name", &name_obj) &&
              json_object_object_get_ex(probe_obj, "functions", &funcs_obj) &&
              json_object_get_type(name_obj) == json_type_string &&
              json_object_get_type(funcs_obj) == json_type_array) {
            std::string probe_name = json_object_get_string(name_obj);
            std::unordered_set<std::string> func_set;
            int func_len = json_object_array_length(funcs_obj);
            for (int j = 0; j < func_len; ++j) {
              json_object* func_obj = json_object_array_get_idx(funcs_obj, j);
              if (func_obj && json_object_get_type(func_obj) == json_type_string) {
                func_set.insert(json_object_get_string(func_obj));
              }
            }
            // Merge with existing exclusion map if present
            auto& existing_set = exclusionMap[probe_name];
            existing_set.insert(func_set.begin(), func_set.end());
          }
        }
      }
      if (jobj) json_object_put(jobj);
    } else {
      DC_LOG_ERROR("Failed to open invalid probes file: %s",
                   configManager_->probe_invalid_file_path.string().c_str());
    }
  }

  static std::unordered_set<std::string> global_function_names;
  std::vector<std::shared_ptr<Probe>> probes;
  // Iterate over all capture probes from configuration
  for (const auto& capture_probe : configManager_->capture_probes) {
    std::vector<std::string> functionNames;
    std::shared_ptr<Probe> probe;

    // Instantiate the correct probe type
    switch (capture_probe->probe_type) {
      case ProbeType::UPROBE:
        probe = std::make_shared<UProbe>();
        break;
      case ProbeType::SYSCALLS:
        probe = std::make_shared<SysCallProbe>();
        break;
      case ProbeType::USDT:
        probe = std::make_shared<USDTProbe>();
        break;
      case ProbeType::KPROBE:
        probe = std::make_shared<KProbe>();
        break;
      case ProbeType::CUSTOM:
        probe = std::make_shared<CustomProbe>();
        break;
      default:
        DC_LOG_ERROR("Unknown probe type encountered in extractProbes()");
        throw std::runtime_error("Unknown probe type encountered in extractProbes()");
    }

    // Extract function names based on capture type
    switch (capture_probe->type) {
      case CaptureType::HEADER:
        DC_LOG_INFO("Extracting header probes...");
        if (auto headerProbe = std::static_pointer_cast<HeaderCaptureProbe>(capture_probe)) {
          DC_LOG_DEBUG("Header Name: %s", headerProbe->file.c_str());
          functionNames = HeaderFunctionExtractor(headerProbe->file).extractFunctionNames();
        }
        if (capture_probe->probe_type == ProbeType::KPROBE) {
          DC_LOG_DEBUG("KPROBE: Extracting symbols from header...");
          const auto& ksym_functions =
              datacrumbs::Singleton<KSymCapture>::get_instance()->functions_;
          std::vector<std::string> validFunctionNames;
          for (const auto& name : functionNames) {
            if (ksym_functions.find(name) != ksym_functions.end()) {
              validFunctionNames.push_back(name);
            } else {
              DC_LOG_WARN("Function '%s' not found in KSymCapture functions, skipping.",
                          name.c_str());
            }
          }
          functionNames = std::move(validFunctionNames);
        }
        break;
      case CaptureType::BINARY:
        DC_LOG_INFO("Extracting binary probes...");
        if (auto binaryProbe = std::static_pointer_cast<BinaryCaptureProbe>(capture_probe)) {
          DC_LOG_DEBUG("Binary Path: %s", binaryProbe->file.c_str());
          functionNames =
              ElfSymbolExtractor(binaryProbe->file, binaryProbe->include_offsets).extract_symbols();
          if (capture_probe->probe_type == ProbeType::UPROBE) {
            DC_LOG_DEBUG("UPROBE: Extracting symbols from binary...");
            if (auto uprobe = std::dynamic_pointer_cast<UProbe>(probe)) {
              uprobe->binary_path = binaryProbe->file;
              uprobe->include_offsets = binaryProbe->include_offsets;
            }
          }
        }
        break;
      case CaptureType::USDT:
        DC_LOG_INFO("Extracting USDT probes...");
        if (auto usdtProbe = std::static_pointer_cast<USDTCaptureProbe>(capture_probe)) {
          if (capture_probe->probe_type == ProbeType::USDT) {
            DC_LOG_DEBUG("USDT: Extracting symbols from binary...");
            if (auto usdt_probe = std::dynamic_pointer_cast<USDTProbe>(probe)) {
              usdt_probe->binary_path = usdtProbe->binary_path;
              usdt_probe->provider = usdtProbe->provider;

              functionNames = USDTFunctionExtractor(usdtProbe->provider).extractFunctionNames();
            }
          }
        }
        break;
      case CaptureType::KSYM:
        DC_LOG_INFO("Extracting kernel symbol probes...");
        if (auto ksymProbe = std::static_pointer_cast<KernelCaptureProbe>(capture_probe)) {
          functionNames = datacrumbs::Singleton<KSymCapture>::get_instance()->getFunctionsByRegex(
              ksymProbe->regex);
        }
        break;
      case CaptureType::CUSTOM:
        DC_LOG_INFO("Extracting custom probes...");
        if (auto customProbe = std::static_pointer_cast<CustomCaptureProbe>(capture_probe)) {
          // Load custom BPF file and extract functions
          if (!std::filesystem::exists(customProbe->bpf_file)) {
            DC_LOG_ERROR("Custom BPF file does not exist: %s", customProbe->bpf_file.c_str());
            has_invalid_probes_ = true;
            continue;  // Skip this probe if the file is missing
          }
          if (!std::filesystem::exists(customProbe->probe_file)) {
            DC_LOG_ERROR("Custom probe file does not exist: %s", customProbe->probe_file.c_str());
            has_invalid_probes_ = true;
            continue;  // Skip this probe if the file is missing
          }
          if (!std::filesystem::exists(customProbe->process_header)) {
            DC_LOG_ERROR("Custom process header file does not exist: %s",
                         customProbe->process_header.c_str());
            has_invalid_probes_ = true;
            continue;  // Skip this probe if the file is missing
          }
          std::ifstream probe_ifs(customProbe->probe_file);
          if (!probe_ifs.is_open()) {
            DC_LOG_ERROR("Failed to open custom probe file: %s", customProbe->probe_file.c_str());
            break;
          }
          std::string probe_content((std::istreambuf_iterator<char>(probe_ifs)),
                                    std::istreambuf_iterator<char>());
          json_object* probe_jobj = json_tokener_parse(probe_content.c_str());
          if (probe_jobj && json_object_get_type(probe_jobj) == json_type_array) {
            int arr_len = json_object_array_length(probe_jobj);
            for (int i = 0; i < arr_len; ++i) {
              json_object* entry = json_object_array_get_idx(probe_jobj, i);
              if (!entry) continue;
              json_object* funcs_obj = nullptr;
              if (json_object_object_get_ex(entry, "functions", &funcs_obj) &&
                  json_object_get_type(funcs_obj) == json_type_array) {
                int func_len = json_object_array_length(funcs_obj);
                for (int j = 0; j < func_len; ++j) {
                  json_object* func_obj = json_object_array_get_idx(funcs_obj, j);
                  if (func_obj && json_object_get_type(func_obj) == json_type_string) {
                    functionNames.push_back(json_object_get_string(func_obj));
                  }
                }
              }
            }
          }
          if (probe_jobj) json_object_put(probe_jobj);
          if (auto custom_probe = std::dynamic_pointer_cast<CustomProbe>(probe)) {
            custom_probe->bpf_path = customProbe->bpf_file;
            custom_probe->start_event_id = customProbe->start_event_id;
            custom_probe->process_header = customProbe->process_header;
            custom_probe->event_type = customProbe->event_type;
          }
        }
        break;
      default:
        DC_LOG_WARN("Unknown capture type encountered!");
    }

    // Filter function names by regex if specified
    if (!capture_probe->regex.empty()) {
      std::regex re(capture_probe->regex, std::regex_constants::icase);
      std::vector<std::string> filteredNames;
      for (const auto& name : functionNames) {
        if (std::regex_match(name, re)) {
          filteredNames.push_back(name);
        }
      }
      functionNames = std::move(filteredNames);
    }

    probe->name = capture_probe->name;

    // For syscall probes, strip "sys_" prefix
    if (capture_probe->probe_type == ProbeType::SYSCALLS) {
      for (auto& name : functionNames) {
        if (name.rfind("sys_", 0) == 0) {
          name = name.substr(4);
        }
      }
    }

    // Exclude functions as per exclusion map
    if (!exclusionMap.empty()) {
      auto it = exclusionMap.find(capture_probe->name);
      if (it != exclusionMap.end()) {
        const auto& excludedFuncs = it->second;
        std::vector<std::string> filteredNames;
        for (const auto& name : functionNames) {
          auto pos = name.find(':');
          std::string base_name = (pos != std::string::npos) ? name.substr(0, pos) : name;
          if (excludedFuncs.find(name) == excludedFuncs.end() &&
              excludedFuncs.find(base_name) == excludedFuncs.end()) {
            filteredNames.push_back(name);
          } else {
            DC_LOG_INFO("Excluding function '%s' from probe '%s' as per exclusion list.",
                        name.c_str(), capture_probe->name.c_str());
          }
        }
        functionNames = std::move(filteredNames);
      }
    }
    if (capture_probe->probe_type != ProbeType::CUSTOM) {
      std::sort(functionNames.begin(), functionNames.end());
    }

    switch (capture_probe->type) {
      case CaptureType::HEADER: {
        DC_LOG_INFO("Deduplicating header probes...");
        std::vector<std::string> validFunctionNames;
        for (const auto& name : functionNames) {
          DC_LOG_INFO("[ProbeExplorer] Function name '%s' from %s.", name.c_str(),
                      capture_probe->name.c_str());
          auto combined_name = name;
          // Check and insert into global set to avoid duplicates
          if (!global_function_names.insert(combined_name).second) {
            DC_LOG_WARN(
                "[ProbeExplorer] Function name '%s' already processed. Skipping duplicate "
                "from %s.",
                name.c_str(), capture_probe->name.c_str());
            continue;
          }
          validFunctionNames.push_back(name);
        }
        functionNames = std::move(validFunctionNames);

        break;
      }
      case CaptureType::BINARY: {
        DC_LOG_INFO("Deduplicating binary probes...");
        if (auto binaryProbe = std::static_pointer_cast<BinaryCaptureProbe>(capture_probe)) {
          if (capture_probe->probe_type == ProbeType::UPROBE) {
            std::vector<std::string> validFunctionNames;
            for (const auto& name : functionNames) {
              auto combined_name = binaryProbe->file + "_" + name;
              // Check and insert into global set to avoid duplicates
              if (!global_function_names.insert(combined_name).second) {
                DC_LOG_WARN(
                    "[ProbeExplorer] Function name '%s' already processed. Skipping duplicate "
                    "from %s.",
                    name.c_str(), capture_probe->name.c_str());
                continue;
              }
              validFunctionNames.push_back(name);
            }
            functionNames = std::move(validFunctionNames);
          }
        }
        break;
      }
      case CaptureType::USDT: {
        DC_LOG_INFO("Deduplicating USDT probes...");
        if (auto usdtProbe = std::static_pointer_cast<USDTCaptureProbe>(capture_probe)) {
          if (capture_probe->probe_type == ProbeType::USDT) {
            std::vector<std::string> validFunctionNames;
            for (const auto& name : functionNames) {
              auto combined_name = usdtProbe->binary_path + "_" + usdtProbe->provider + "_" + name;
              // Check and insert into global set to avoid duplicates
              if (!global_function_names.insert(combined_name).second) {
                DC_LOG_WARN(
                    "[ProbeExplorer] Function name '%s' already processed. Skipping duplicate "
                    "from %s.",
                    name.c_str(), capture_probe->name.c_str());
                continue;
              }
              validFunctionNames.push_back(name);
            }
            functionNames = std::move(validFunctionNames);
          }
        }
        break;
      }
      case CaptureType::KSYM: {
        DC_LOG_INFO("Deduplicating kernel symbol probes...");
        if (auto ksymProbe = std::static_pointer_cast<KernelCaptureProbe>(capture_probe)) {
          std::vector<std::string> validFunctionNames;
          for (const auto& name : functionNames) {
            auto combined_name = name;
            // Check and insert into global set to avoid duplicates
            if (!global_function_names.insert(combined_name).second) {
              DC_LOG_WARN(
                  "[ProbeExplorer] Function name '%s' already processed. Skipping duplicate "
                  "from %s.",
                  name.c_str(), capture_probe->name.c_str());
              continue;
            }
            validFunctionNames.push_back(name);
          }
          functionNames = std::move(validFunctionNames);
        }
        break;
      }
      case CaptureType::CUSTOM: {
        DC_LOG_INFO("Deduplicating custom probes...");
        std::vector<std::string> validFunctionNames;
        for (const auto& name : functionNames) {
          DC_LOG_DEBUG("[ProbeExplorer] Function name '%s' from %s.", name.c_str(),
                       capture_probe->name.c_str());
          auto combined_name = name;
          // Check and insert into global set to avoid duplicates
          if (!global_function_names.insert(combined_name).second) {
            DC_LOG_WARN(
                "[ProbeExplorer] Function name '%s' already processed. Skipping duplicate "
                "from %s.",
                name.c_str(), capture_probe->name.c_str());
            continue;
          }
          validFunctionNames.push_back(name);
        }
        functionNames = std::move(validFunctionNames);
        break;
      }
      default:
        DC_LOG_WARN("Unknown capture type encountered!");
    }

    probe->functions = functionNames;

    // Validate the probe before adding
    if (!probe->validate()) {
      DC_LOG_ERROR("Probe validation failed for: %s", probe->name.c_str());
      has_invalid_probes_ = true;
      continue;  // Skip invalid probes
    }
    DC_LOG_INFO("Valid probe extracted: %s", probe->name.c_str());
    probes.push_back(probe);
  }
  if (has_invalid_probes_) {
    DC_LOG_ERROR("One or more probes failed validation. Please check the logs above.");
  }
  DC_LOG_TRACE("ProbeExplorer::extractProbes - end");
  return probes;
}
void ProbeExplorer::create_exclusion_file(std::vector<std::shared_ptr<Probe>> probes) {
  DC_LOG_TRACE("ProbeExplorer::create_exclusion_file - start");
  json_object* jexarray = json_object_new_array();
  // Serialize each probe to JSON without functions
  for (const auto& probe : probes) {
    json_object* jexclude = nullptr;
    switch (probe->type) {
      case ProbeType::SYSCALLS:
        jexclude = std::dynamic_pointer_cast<SysCallProbe>(probe)->toJson(false);
        break;
      case ProbeType::KPROBE:
        jexclude = std::dynamic_pointer_cast<KProbe>(probe)->toJson(false);
        break;
      case ProbeType::UPROBE:
        jexclude = std::dynamic_pointer_cast<UProbe>(probe)->toJson(false);
        break;
      case ProbeType::USDT:
        jexclude = std::dynamic_pointer_cast<USDTProbe>(probe)->toJson(false);
        break;
      case ProbeType::CUSTOM:
        jexclude = std::dynamic_pointer_cast<CustomProbe>(probe)->toJson(false);
        break;
      default:
        DC_LOG_ERROR("Unknown probe type encountered.");
        continue;  // Skip unknown types
    }
    if (!jexclude) {
      DC_LOG_ERROR("Failed to serialize probe for exclusion: %s", probe->name.c_str());
      continue;  // Skip serialization failure
    }
    json_object_array_add(jexarray, jexclude);
  }
  if (!configManager_->probe_exclusion_file_path.empty() &&
      !std::filesystem::exists(configManager_->probe_exclusion_file_path)) {
    const char* exclude_json_str =
        json_object_to_json_string_ext(jexarray, JSON_C_TO_STRING_PRETTY);
    std::ofstream ofs(configManager_->probe_exclusion_file_path);
    if (ofs.is_open()) {
      ofs << exclude_json_str;
      ofs.close();
    } else {
      DC_LOG_ERROR("Failed to open file: %s", configManager_->probe_exclusion_file_path.c_str());
    }
  }

  DC_LOG_TRACE("ProbeExplorer::create_exclusion_file - end");
}

// Writes extracted probes to a JSON file and returns the probe list
std::vector<std::shared_ptr<Probe>> ProbeExplorer::writeProbesToJson() {
  DC_LOG_TRACE("ProbeExplorer::writeProbesToJson - start");
  auto probes = extractProbes();
  if (probes.empty()) {
    DC_LOG_WARN("No valid probes extracted. Skipping JSON write.");
    return probes;
  }
  if (!configManager_->probe_exclusion_file_path.empty() &&
      !std::filesystem::exists(configManager_->probe_exclusion_file_path)) {
    create_exclusion_file(probes);
  }
  json_object* jarray = json_object_new_array();

  // Serialize each probe to JSON
  for (const auto& probe : probes) {
    json_object* jprobe = nullptr;
    switch (probe->type) {
      case ProbeType::SYSCALLS:
        jprobe = std::dynamic_pointer_cast<SysCallProbe>(probe)->toJson();
        break;
      case ProbeType::KPROBE:
        jprobe = std::dynamic_pointer_cast<KProbe>(probe)->toJson();
        break;
      case ProbeType::UPROBE:
        jprobe = std::dynamic_pointer_cast<UProbe>(probe)->toJson();
        break;
      case ProbeType::USDT:
        jprobe = std::dynamic_pointer_cast<USDTProbe>(probe)->toJson();
        break;
      case ProbeType::CUSTOM:
        jprobe = std::dynamic_pointer_cast<CustomProbe>(probe)->toJson();
        break;
      default:
        DC_LOG_ERROR("Unknown probe type encountered.");
        continue;  // Skip unknown types
    }
    if (!jprobe) {
      DC_LOG_ERROR("Failed to serialize probe: %s", probe->name.c_str());
      continue;  // Skip serialization failure
    }
    json_object_array_add(jarray, jprobe);
  }

  // Write JSON to file
  const char* json_str = json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PRETTY);

  std::ofstream ofs(configManager_->probe_file_path);
  if (ofs.is_open()) {
    ofs << json_str;
    ofs.close();
  } else {
    DC_LOG_ERROR("Failed to open file: %s", configManager_->probe_file_path.c_str());
  }

  json_object_put(jarray);  // free memory
  DC_LOG_TRACE("ProbeExplorer::writeProbesToJson - end");
  return probes;
}

}  // namespace datacrumbs

/**
 * Example usage:
 * g++ -std=c++14 /home/haridev/datacrumbs/src/datacrumbs/common/configuration_manager.cpp
 * probe_explorer_test.cpp probe_explorer.cpp mechanism/ksym_capture.cpp -o probe_explorer_test
 * -I/home/haridev/datacrumbs/src -lelf `llvm-config --cxxflags  --ldflags --system-libs --libs
 * core` -lclang -lyaml-cpp
 */

// Main function to run the probe explorer and print extracted probes
int main(int argc, char** argv) {
  DC_LOG_TRACE("main - start");
  datacrumbs::utils::Timer timer;  // Create timer instance
  timer.resumeTime();              // Start timing

  datacrumbs::ProbeExplorer explorer(argc, argv);
  auto probes = explorer.writeProbesToJson();

  // Print probe names and a sample of their functions
  for (const auto& probe : probes) {
    DC_LOG_INFO("Probe: %s", probe->name.c_str());
    int i = 0;
    for (const auto& value : probe->functions) {
      DC_LOG_DEBUG("  Value: %s", value.c_str());
      if (i++ > 10) {
        DC_LOG_DEBUG("  ... (truncated)");
        break;
      }
    }
  }

  timer.pauseTime();  // Stop timer and accumulate elapsed time
  DC_LOG_PRINT("Elapsed time in Probe Explorer: %f seconds", timer.getElapsedTime());
  DC_LOG_TRACE("main - end");
  if (explorer.has_invalid_probes_) {
    DC_LOG_ERROR("Probe exploration completed with errors due to invalid probes.");
    return 1;  // Indicate error due to invalid probes
  }
  return 0;
}