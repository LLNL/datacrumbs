#include <iostream>
#include <string>
#include <vector>

#include "yaml_utils.h"

// Simple CLI that maps first positional arg to function name.
// Usage examples:
//   yaml_query capture_probes_names
//   yaml_query capture_probes_names --config-path /path/to/config.yaml

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0]
              << " <function_name> [--config-path <path>]" << std::endl;
    return 1;
  }
  std::string func = argv[1];
  std::string config_path;

  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--config-path" && i + 1 < argc) {
      config_path = argv[++i];
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      return 2;
    }
  }

  try {
    if (config_path.empty()) {
      std::cerr << "Error: --config-path is required" << std::endl;
      return 2;
    }
    if (func == "capture_probes_names") {
      auto names = datacrumbs::utilities::capture_probes_names(config_path);
      for (const auto& n : names) {
        std::cout << n << std::endl;
      }
      return 0;
    }
    std::cerr << "Unknown function name: " << func << std::endl;
    return 3;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 10;
  }
}
