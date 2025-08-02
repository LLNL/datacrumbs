#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/explorer/mechanism/elf_capture.h>
#include <datacrumbs/explorer/mechanism/header_capture.h>
#include <datacrumbs/explorer/mechanism/ksym_capture.h>
#include <datacrumbs/explorer/mechanism/usdt_functions.h>
#include <datacrumbs/explorer/probe_explorer.h>

#include <regex>
#include <unordered_map>
#include <unordered_set>
namespace datacrumbs {
ProbeExplorer::ProbeExplorer(int argc, char** argv) {
  configManager_ = datacrumbs::Singleton<ConfigurationManager>::get_instance(argc, argv);
}
std::vector<std::shared_ptr<Probe>> ProbeExplorer::extractProbes() {
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
            exclusionMap[probe_name] = std::move(func_set);
          }
        }
      }
      if (jobj) json_object_put(jobj);
    } else {
      std::cerr << "Failed to open exclusion probes file: " << configManager_->exclusion_probes
                << std::endl;
    }
  }
  std::vector<std::shared_ptr<Probe>> probes;
  for (const auto& capture_probe : configManager_->capture_probes) {
    std::vector<std::string> functionNames;
    std::shared_ptr<Probe> probe;
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
      default:
        throw std::runtime_error("Unknown probe type encountered in extractProbes()");
        break;
    }
    switch (capture_probe->type) {
      case CaptureType::HEADER:
        std::cout << "Extracting header probes..." << std::endl;
        if (auto headerProbe = std::static_pointer_cast<HeaderCaptureProbe>(capture_probe)) {
          std::cout << "Header Name: " << headerProbe->file << std::endl;
          functionNames = HeaderFunctionExtractor(headerProbe->file).extractFunctionNames();
        }
        break;
      case CaptureType::BINARY:
        std::cout << "Extracting binary probes..." << std::endl;
        if (auto binaryProbe = std::static_pointer_cast<BinaryCaptureProbe>(capture_probe)) {
          std::cout << "Binary Path: " << binaryProbe->file << std::endl;
          functionNames = ElfSymbolExtractor(binaryProbe->file).extract_symbols();
          if (capture_probe->probe_type == ProbeType::UPROBE) {
            std::cout << "UPROBE: Extracting symbols from binary..." << std::endl;
            if (auto uprobe = std::dynamic_pointer_cast<UProbe>(probe)) {
              uprobe->binary_path = binaryProbe->file;
            }
          }
        }
        break;
      case CaptureType::USDT:
        std::cout << "Extracting USDT probes..." << std::endl;
        if (auto usdtProbe = std::static_pointer_cast<USDTCaptureProbe>(capture_probe)) {
          if (capture_probe->probe_type == ProbeType::USDT) {
            std::cout << "USDT: Extracting symbols from binary..." << std::endl;
            if (auto usdt_probe = std::dynamic_pointer_cast<USDTProbe>(probe)) {
              usdt_probe->binary_path = usdtProbe->binary_path;
              usdt_probe->provider = usdtProbe->provider;
            }
            functionNames = USDTFunctionExtractor(usdtProbe->provider).extractFunctionNames();
          }
        }
        break;
      case CaptureType::KSYM:
        std::cout << "Extracting kernel symbol probes..." << std::endl;
        if (auto ksymProbe = std::static_pointer_cast<KernelCaptureProbe>(capture_probe)) {
          functionNames = datacrumbs::Singleton<KSymCapture>::get_instance()->getFunctionsByRegex(
              ksymProbe->regex);
        }
        break;
      default:
        std::cerr << "Unknown probe type!" << std::endl;
    }

    if (!exclusionMap.empty()) {
      auto it = exclusionMap.find(capture_probe->name);
      if (it != exclusionMap.end()) {
        const auto& excludedFuncs = it->second;
        std::vector<std::string> filteredNames;
        for (const auto& name : functionNames) {
          if (excludedFuncs.find(name) == excludedFuncs.end()) {
            filteredNames.push_back(name);
          }
        }
        functionNames = std::move(filteredNames);
      }
    }
    if (!capture_probe->regex.empty()) {
      std::regex re(capture_probe->regex);
      std::vector<std::string> filteredNames;
      for (const auto& name : functionNames) {
        if (std::regex_match(name, re)) {
          filteredNames.push_back(name);
        }
      }
      functionNames = std::move(filteredNames);
    }

    probe->name = capture_probe->name;
    if (capture_probe->probe_type == ProbeType::SYSCALLS) {
      for (auto& name : functionNames) {
        if (name.rfind("sys_", 0) == 0) {
          name = name.substr(4);
        }
      }
    }
    probe->functions = functionNames;
    // for (const auto& name : functionNames) {
    //   std::cout << "Function: " << name << std::endl;
    // }
    if (!probe->validate()) {
      std::cerr << "Probe validation failed for: " << probe->name << std::endl;
      continue;  // Skip invalid probes
    }
    std::cout << "Valid probe extracted: " << probe->name << std::endl;
    // Add the probe to the list
    probes.push_back(probe);
  }
  return probes;
}

std::vector<std::shared_ptr<Probe>> ProbeExplorer::writeProbesToJson() {
  auto probes = extractProbes();
  json_object* jarray = json_object_new_array();

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
      default:
        std::cerr << "Unknown probe type encountered." << std::endl;
        continue;  // Skip unknown types
    }
    json_object_array_add(jarray, jprobe);
  }

  const char* json_str = json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PRETTY);

  std::ofstream ofs(configManager_->probe_file_path);
  if (ofs.is_open()) {
    ofs << json_str;
    ofs.close();
  } else {
    std::cerr << "Failed to open file: " << configManager_->probe_file_path << std::endl;
  }

  json_object_put(jarray);  // free memory
  return probes;
}
}  // namespace datacrumbs

/**
 * g++ -std=c++14 /home/haridev/datacrumbs/src/datacrumbs/common/configuration_manager.cpp
 * probe_explorer_test.cpp probe_explorer.cpp mechanism/ksym_capture.cpp -o probe_explorer_test
 * -I/home/haridev/datacrumbs/src -lelf `llvm-config --cxxflags  --ldflags --system-libs --libs
 * core` -lclang -lyaml-cpp
 */

int main(int argc, char** argv) {
  datacrumbs::ProbeExplorer explorer(argc, argv);
  auto probes = explorer.writeProbesToJson();

  for (const auto& probe : probes) {
    std::cout << "Probe: " << probe->name << "\n";
    int i = 0;
    for (const auto& value : probe->functions) {
      std::cout << "  Value: " << value << "\n";
      if (i++ > 10) {
        std::cout << "  ... (truncated)" << std::endl;
        break;
      }
    }
  }
  return 0;
}