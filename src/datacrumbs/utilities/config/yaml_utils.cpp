#include "yaml_utils.h"

#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include <datacrumbs/common/enumerations.h>
#include <datacrumbs/common/utils.h>
#include <datacrumbs/common/data_structures.h>

namespace datacrumbs {
namespace utilities {

static YAML::Node load_yaml_file(const std::string& path) {
  try {
    return YAML::LoadFile(path);
  } catch (const YAML::BadFile& e) {
    throw std::runtime_error(std::string("Failed to load YAML file: ") + path + "; error: " +
                             e.what());
  } catch (const YAML::ParserException& e) {
    throw std::runtime_error(std::string("Failed to parse YAML file: ") + path + "; error: " +
                             e.what());
  }
}

std::vector<std::string> capture_probes_names(const std::string& config_path) {
  YAML::Node config = load_yaml_file(config_path);

  std::vector<std::string> names;
  if (!config || !config["capture_probes"]) {
    return names;  // empty
  }

  const YAML::Node& probes = config["capture_probes"];
  if (!probes.IsSequence()) {
    throw std::runtime_error("YAML key 'capture_probes' must be a sequence (list).");
  }

  for (const auto& probe_node : probes) {
    if (capture_probe_disabled(probe_node)) continue;
    if (probe_node["name"]) {
      names.emplace_back(probe_node["name"].as<std::string>());
    }
  }
  return names;
}

std::vector<std::shared_ptr<CaptureProbe>> capture_probes(const std::string& config_path) {
  YAML::Node config = load_yaml_file(config_path);

  std::vector<std::shared_ptr<CaptureProbe>> out;
  if (!config || !config["capture_probes"]) {
    return out;  // empty
  }

  const YAML::Node& probes = config["capture_probes"];
  if (!probes.IsSequence()) {
    throw std::runtime_error("YAML key 'capture_probes' must be a sequence (list).");
  }

  for (const auto& probe_node : probes) {
    if (capture_probe_disabled(probe_node)) {
      continue;
    }

    if (!probe_node["type"]) {
      throw std::invalid_argument("Capture probe entry missing required key 'type'.");
    }

    CaptureType type;
    convert(probe_node["type"].as<std::string>(), type);

    std::shared_ptr<CaptureProbe> probe;

    switch (type) {
      case CaptureType::HEADER: {
        auto header_probe = std::make_shared<HeaderCaptureProbe>();
        if (probe_node["file"]) {
          header_probe->file = probe_node["file"].as<std::string>();
        } else {
          throw std::invalid_argument("Header capture probe requires 'file'.");
        }
        probe = std::static_pointer_cast<CaptureProbe>(header_probe);
        break;
      }
      case CaptureType::BINARY: {
        auto binary_probe = std::make_shared<BinaryCaptureProbe>();
        if (probe_node["file"]) {
          binary_probe->file = probe_node["file"].as<std::string>();
        } else {
          throw std::invalid_argument("Binary capture probe requires 'file'.");
        }
        if (probe_node["include_offsets"]) {
          binary_probe->include_offsets = probe_node["include_offsets"].as<bool>();
        } else {
          binary_probe->include_offsets = false;
        }
        probe = std::static_pointer_cast<CaptureProbe>(binary_probe);
        break;
      }
      case CaptureType::KSYM: {
        auto ksym_probe = std::make_shared<KernelCaptureProbe>();
        if (probe_node["regex"]) {
          ksym_probe->regex = probe_node["regex"].as<std::string>();
        } else {
          throw std::invalid_argument("KSYM capture probe requires 'regex'.");
        }
        probe = std::static_pointer_cast<CaptureProbe>(ksym_probe);
        break;
      }
      case CaptureType::USDT: {
        auto usdt_probe = std::make_shared<USDTCaptureProbe>();
        if (probe_node["binary_path"]) {
          usdt_probe->binary_path = probe_node["binary_path"].as<std::string>();
        } else {
          throw std::invalid_argument("USDT capture probe requires 'binary_path'.");
        }
        if (probe_node["provider"]) {
          usdt_probe->provider = probe_node["provider"].as<std::string>();
        } else {
          throw std::invalid_argument("USDT capture probe requires 'provider'.");
        }
        probe = std::static_pointer_cast<CaptureProbe>(usdt_probe);
        break;
      }
      case CaptureType::CUSTOM: {
        auto custom_probe = std::make_shared<CustomCaptureProbe>();
        if (probe_node["file"]) {
          custom_probe->bpf_file = probe_node["file"].as<std::string>();
        } else {
          throw std::invalid_argument("CUSTOM capture probe requires 'file'.");
        }
        if (probe_node["probes"]) {
          custom_probe->probe_file = probe_node["probes"].as<std::string>();
        } else {
          throw std::invalid_argument("CUSTOM capture probe requires 'probes'.");
        }
        if (probe_node["start_event_id"]) {
          custom_probe->start_event_id = probe_node["start_event_id"].as<uint64_t>();
        }
        if (probe_node["process_header"]) {
          custom_probe->process_header = probe_node["process_header"].as<std::string>();
        }
        if (probe_node["event_type"]) {
          custom_probe->event_type = probe_node["event_type"].as<uint64_t>();
        } else {
          custom_probe->event_type = 1;  // default as in existing code
        }
        probe = std::static_pointer_cast<CaptureProbe>(custom_probe);
        break;
      }
      default:
        throw std::invalid_argument("Unknown CaptureType in configuration: " +
                                    probe_node["type"].as<std::string>());
    }

    // Common fields
    if (probe_node["enable_explorer"]) {
      probe->enable_explorer = probe_node["enable_explorer"].as<bool>();
    } else {
      probe->enable_explorer = true;
    }

    if (probe_node["probe"]) {
      auto probe_type_str = probe_node["probe"].as<std::string>();
      convert(probe_type_str, probe->probe_type);
    } else {
      throw std::invalid_argument("Probe type ('probe') is required for capture probe.");
    }

    if (probe_node["name"]) {
      probe->name = probe_node["name"].as<std::string>();
    } else {
      throw std::invalid_argument("Probe 'name' is required for capture probe.");
    }

    if (probe_node["regex"]) {
      probe->regex = probe_node["regex"].as<std::string>();
    }

    out.push_back(probe);
  }

  return out;
}

}  // namespace utilities
}  // namespace datacrumbs
