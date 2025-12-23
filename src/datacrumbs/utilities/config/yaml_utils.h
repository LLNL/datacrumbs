// Lightweight YAML utility functions for DataCrumbs configs
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <yaml-cpp/yaml.h>
#include <datacrumbs/common/data_structures.h>

namespace datacrumbs {
namespace utilities {

// Returns list of capture probe names from YAML, skipping entries
// with `disable: true`. `config_path` defaults to the ares.yaml path.
std::vector<std::string> capture_probes_names(
    const std::string& config_path);

// Helper to check if a capture probe node is disabled.
inline bool capture_probe_disabled(const YAML::Node& probe_node) {
    return probe_node["disable"] && probe_node["disable"].as<bool>();
}

// Build full capture probe objects from YAML, skipping disabled entries.
// Mirrors the parsing logic used in ConfigurationManager.
// Throws on malformed input.
std::vector<std::shared_ptr<CaptureProbe>> capture_probes(
        const std::string& config_path);

}  // namespace utilities
}  // namespace datacrumbs
