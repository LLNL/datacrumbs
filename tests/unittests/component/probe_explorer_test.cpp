#include <datacrumbs/explorer/probe_explorer.h>

int main(int argc, char** argv) {
  datacrumbs::ProbeExplorer explorer(argc, argv);
  auto probes = explorer.extractProbes();

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

/**
 * g++ -std=c++14 probe_explorer_test.cpp probe_explorer.cpp -o probe_explorer_test
 * -I/home/haridev/datacrumbs/src -lelf `llvm-config --cxxflags  --ldflags --system-libs --libs
 * core` -lclang
 */