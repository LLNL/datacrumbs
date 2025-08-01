#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/explorer/mechanism/elf_capture.h>
#include <datacrumbs/explorer/mechanism/header_capture.h>
#include <datacrumbs/explorer/mechanism/ksym_capture.h>
#include <datacrumbs/explorer/mechanism/usdt_functions.h>
#include <datacrumbs/explorer/probe_explorer.h>

#include <regex>
namespace datacrumbs {
ProbeExplorer::ProbeExplorer(int argc, char** argv) {
  configManager_ = datacrumbs::Singleton<ConfigurationManager>::get_instance(argc, argv);
}
std::vector<std::shared_ptr<Probe>> ProbeExplorer::extractProbes() {
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