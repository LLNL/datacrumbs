#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/explorer/mechanism/elf_capture.h>
#include <datacrumbs/explorer/mechanism/header_capture.h>
#include <datacrumbs/explorer/mechanism/ksym_capture.h>
#include <datacrumbs/explorer/probe_explorer.h>

#include <regex>
namespace datacrumbs {
ProbeExplorer::ProbeExplorer(int argc, char** argv) {
  configManager_ = datacrumbs::Singleton<ConfigurationManager>::get_instance(argc, argv);
}
std::vector<Probe> ProbeExplorer::extractProbes() {
  std::vector<Probe> probes;
  for (const auto& capture_probe : configManager_->capture_probes) {
    std::vector<std::string> functionNames;
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
    Probe probe(capture_probe->probe_type);
    probe.name = capture_probe->name;
    probe.intercept_name = capture_probe->name;
    std::replace(probe.intercept_name.begin(), probe.intercept_name.end(), '.', '_');
    if (capture_probe->probe_type == ProbeType::SYSCALLS) {
      for (auto& name : functionNames) {
        if (name.rfind("sys_", 0) == 0) {
          name = name.substr(4);
        }
      }
    }
    probe.functions = functionNames;
    // for (const auto& name : functionNames) {
    //   std::cout << "Function: " << name << std::endl;
    // }
    probes.push_back(probe);
  }
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
  auto probes = explorer.writeProbesToJson("probes.json");

  for (const auto& probe : probes) {
    std::cout << "Probe: " << probe.name << "\n";
    int i = 0;
    for (const auto& value : probe.functions) {
      std::cout << "  Value: " << value << "\n";
      if (i++ > 10) {
        std::cout << "  ... (truncated)" << std::endl;
        break;
      }
    }
  }
  return 0;
}