
#include <datacrumbs/generator/generator.h>
namespace datacrumbs {

// Constructor for ProbeGenerator
ProbeGenerator::ProbeGenerator(int argc, char** argv) : eventIdCounter_(1000) {
  // Initialize ConfigurationManager singleton
  configManager_ = datacrumbs::Singleton<ConfigurationManager>::get_instance(argc, argv);
}
int ProbeGenerator::run() {
  DC_LOG_TRACE("[ProbeGenerator] Starting run()");

  if (!configManager_) {
    DC_LOG_ERROR("ConfigurationManager is not initialized.");
    return -1;
  }
  json_object* jarray = json_object_new_array();
  // Global set to track unique function names
  static std::unordered_set<std::string> global_function_names;

  // Get probes file path from configuration
  const auto& probesFile = configManager_->probe_file_path;
  DC_LOG_INFO("[ProbeGenerator] Reading probes file: %s", probesFile.c_str());

  // Read probes JSON file
  struct json_object* probesJson = json_object_from_file(probesFile.c_str());
  if (!probesJson) {
    DC_LOG_ERROR("Failed to read probes file: %s", probesFile.c_str());
    return -1;
  }

  auto probe_files = std::vector<std::string>();
  auto process_headers = std::vector<std::string>();
  int arr_len = json_object_array_length(probesJson);
  DC_LOG_INFO("[ProbeGenerator] Number of probes: %d", arr_len);

  // Track total number of generated probes
  size_t total_probes_generated = 0;
  update_event("M", "SH", 0);
  update_event(DATACRUMBS_PROBE_CATEGORY, START_FUNCTION_NAME, START_EVENT_ID);
  update_event(DATACRUMBS_PROBE_CATEGORY, END_FUNCTION_NAME, END_EVENT_ID);
  bool manual_probe_added = false;
  // Iterate over each probe in the JSON array
  for (int i = 0; i < arr_len; ++i) {
    struct json_object* jprobe = json_object_array_get_idx(probesJson, i);
    auto probe = Probe::fromJson(jprobe);
    std::shared_ptr<Probe> manual_probe;
    switch (probe.type) {
      case ProbeType::UPROBE:
        manual_probe = std::make_shared<UProbe>(UProbe::fromJson(jprobe));
        break;
      case ProbeType::SYSCALLS:
        manual_probe = std::make_shared<SysCallProbe>(SysCallProbe::fromJson(jprobe));
        break;
      case ProbeType::USDT:
        manual_probe = std::make_shared<USDTProbe>(USDTProbe::fromJson(jprobe));
        break;
      case ProbeType::KPROBE:
        manual_probe = std::make_shared<KProbe>(KProbe::fromJson(jprobe));
        break;
      case ProbeType::CUSTOM:
        manual_probe = std::make_shared<CustomProbe>(CustomProbe::fromJson(jprobe));
        break;
      default:
        DC_LOG_ERROR("Unknown probe type encountered in extractProbes()");
        throw std::runtime_error("Unknown probe type encountered in extractProbes()");
    }
    manual_probe->functions.clear();  // Clear functions for manual probe
    DC_LOG_INFO("[ProbeGenerator] Processing probe: %s", probe.name.c_str());

    std::stringstream ss;
    ss << "#include <datacrumbs/server/bpf/common.h>" << std::endl;

    // Iterate over each function in the probe
    for (size_t func_index = 0; func_index < probe.functions.size(); ++func_index) {
      const auto& func = probe.functions[func_index];

      int current_event_id = 1000;
      if (probe.type != ProbeType::CUSTOM) {
        current_event_id = this->eventIdCounter_++;
      } else {
        auto custom = CustomProbe::fromJson(jprobe);
        current_event_id = custom.start_event_id + func_index;
      }

      // Map event id to probe name and function name
      struct json_object* info = json_object_new_object();
      json_object_object_add(info, "probe_name", json_object_new_string(probe.name.c_str()));
      json_object_object_add(info, "function_name", json_object_new_string(func.c_str()));
      categoryMap_[current_event_id] = info;

      // Generate code based on probe type
      switch (probe.type) {
        case ProbeType::KPROBE: {
          auto combined_name = func;
          // Check and insert into global set to avoid duplicates
          if (!global_function_names.insert(combined_name).second) {
            DC_LOG_WARN(
                "[ProbeGenerator] Function name '%s' already processed. Skipping duplicate from "
                "%s.",
                func.c_str(), probe.name.c_str());
            continue;
          }
          DC_LOG_DEBUG("[ProbeGenerator] Using KProbeGenerator for function: %s (event_id: %d)",
                       func.c_str(), current_event_id);
          KProbeGenerator generator(current_event_id, func);
          ss << generator.generate().str() << std::endl;
          break;
        }
        case ProbeType::UPROBE: {
          DC_LOG_DEBUG("[ProbeGenerator] Using UProbeGenerator for function: %s (event_id: %d)",
                       func.c_str(), current_event_id);
          auto uprobe = UProbe::fromJson(jprobe);
          std::string function_name, offset;
          auto pos = func.find(':');
          bool is_manual = false;
          if (pos != std::string::npos) {
            function_name = func.substr(0, pos);
            offset = func.substr(pos + 1);
            is_manual = true;
          } else {
            function_name = func;
            offset = "";
          }
          auto combined_name = uprobe.binary_path + "_" + function_name + "_" + offset;
          // Check and insert into global set to avoid duplicates
          if (!global_function_names.insert(combined_name).second) {
            DC_LOG_WARN(
                "[ProbeGenerator] Function name '%s' already processed. Skipping duplicate from "
                "%s.",
                func.c_str(), probe.name.c_str());
            continue;
          }
          if (is_manual) {
            DC_LOG_DEBUG("[ProbeGenerator] Adding manual uprobe for function: %s", func.c_str());
            manual_probe->functions.push_back(std::to_string(current_event_id));
          }
          // if (!uprobe.include_offsets) offset = "";
          UProbeGenerator uprobe_gen(current_event_id, function_name, offset, uprobe.binary_path,
                                     is_manual);
          ss << uprobe_gen.generate().str() << std::endl;
          break;
        }
        case ProbeType::SYSCALLS: {
          DC_LOG_DEBUG("[ProbeGenerator] Using SyscallGenerator for function: %s (event_id: %d)",
                       func.c_str(), current_event_id);
          SyscallGenerator syscall_gen(current_event_id, func);
          auto combined_name = func;
          // Check and insert into global set to avoid duplicates
          if (!global_function_names.insert(combined_name).second) {
            DC_LOG_WARN(
                "[ProbeGenerator] Function name '%s' already processed. Skipping duplicate from "
                "%s.",
                func.c_str(), probe.name.c_str());
            continue;
          }
          ss << syscall_gen.generate().str() << std::endl;

          break;
        }
        case ProbeType::USDT: {
          DC_LOG_DEBUG("[ProbeGenerator] Using USDTGenerator for function: %s (event_id: %d)",
                       func.c_str(), current_event_id);
          auto usdt = USDTProbe::fromJson(jprobe);
          auto combined_name = usdt.binary_path + "_" + usdt.provider + "_" + func;
          // Check and insert into global set to avoid duplicates
          if (!global_function_names.insert(combined_name).second) {
            DC_LOG_WARN(
                "[ProbeGenerator] Function name '%s' already processed. Skipping duplicate from "
                "%s.",
                func.c_str(), probe.name.c_str());
            continue;
          }
          USDTGenerator usdt_gen(current_event_id, func, usdt.binary_path, usdt.provider);
          ss << usdt_gen.generate().str() << std::endl;
        } break;
        case ProbeType::CUSTOM: {
          auto combined_name = func;
          // Check and insert into global set to avoid duplicates
          if (!global_function_names.insert(combined_name).second) {
            DC_LOG_WARN(
                "[ProbeGenerator] Function name '%s' already processed. Skipping duplicate from "
                "%s.",
                func.c_str(), probe.name.c_str());
            continue;
          }
        } break;
        default: {
          DC_LOG_ERROR("Unknown probe type: %d", static_cast<int>(probe.type));
        }
      }
      // Increment total probes generated
      ++total_probes_generated;
    }
    if (manual_probe != nullptr && !manual_probe->functions.empty()) {
      DC_LOG_INFO("[ProbeGenerator] Manual probe for %d functions added for probe: %s",
                  manual_probe->functions.size(), probe.name.c_str());
      manual_probe_added = true;
      json_object* manual_jprobe = nullptr;
      switch (manual_probe->type) {
        case ProbeType::SYSCALLS:
          manual_jprobe = std::dynamic_pointer_cast<SysCallProbe>(manual_probe)->toJson();
          break;
        case ProbeType::KPROBE:
          manual_jprobe = std::dynamic_pointer_cast<KProbe>(manual_probe)->toJson();
          break;
        case ProbeType::UPROBE: {
          auto manual_uprobe = std::dynamic_pointer_cast<UProbe>(manual_probe);
          DC_LOG_DEBUG("[ProbeGenerator] Adding manual uprobe for function: %s",
                       manual_uprobe->binary_path.c_str());
          manual_jprobe = manual_uprobe->toJson();
          break;
        }
        case ProbeType::USDT:
          manual_jprobe = std::dynamic_pointer_cast<USDTProbe>(manual_probe)->toJson();
          break;
        case ProbeType::CUSTOM:
          manual_jprobe = std::dynamic_pointer_cast<CustomProbe>(manual_probe)->toJson();
          break;
        default:
          DC_LOG_ERROR("Unknown probe type encountered.");
          continue;  // Skip unknown types
      }
      json_object_array_add(jarray, manual_jprobe);
    }
    if (probe.type == ProbeType::CUSTOM) {
      auto custom = CustomProbe::fromJson(jprobe);
      probe_files.push_back(custom.bpf_path);
      process_headers.push_back(custom.process_header);
      ss << "#include \"" << custom.bpf_path << "\"" << std::endl;
    }
    // Write generated code to file
    const char* gen_path = DATACRUMBS_SRC_GEN_PATH;
    if (!gen_path) {
      DC_LOG_ERROR("DATACRUMBS_SRC_GEN_PATH environment variable not set.");
    } else {
      std::filesystem::create_directories(std::filesystem::path(gen_path) /
                                          "datacrumbs/server/bpf");
      std::string filename =
          (std::filesystem::path(gen_path) / "datacrumbs/server/bpf" / (probe.name + ".bpf.c"))
              .string();
      std::ofstream out(filename);
      if (!out) {
        DC_LOG_ERROR("Failed to open file for writing: %s", filename.c_str());
      } else {
        out << ss.str();
        out.close();
        probe_files.push_back(probe.name + ".bpf.c");
        DC_LOG_INFO("[ProbeGenerator] Generated %zu functions file: %s", probe.functions.size(),
                    filename.c_str());
      }
    }
  }
  if (manual_probe_added) {
    // Write JSON to file
    const char* json_str = json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PRETTY);

    std::ofstream ofs(configManager_->manual_probe_path);
    if (ofs.is_open()) {
      ofs << json_str;
      ofs.close();
    } else {
      DC_LOG_ERROR("Failed to open file: %s", configManager_->manual_probe_path.c_str());
    }

    json_object_put(jarray);  // free memory
    DC_LOG_TRACE("ProbeExplorer::writeProbesToJson - end");
  } else {
    DC_LOG_INFO("No manual probes were added.");
    // Remove existing manual probe file if it exists
    std::error_code ec;
    if (std::filesystem::exists(configManager_->manual_probe_path, ec)) {
      std::filesystem::remove(configManager_->manual_probe_path, ec);
      if (ec) {
        DC_LOG_ERROR("Failed to remove file: %s", configManager_->manual_probe_path.c_str());
      } else {
        DC_LOG_INFO("Removed existing manual probe file: %s",
                    configManager_->manual_probe_path.c_str());
      }
    }
  }
  // Append all generated probe files as includes to generated.bpf.c
  const char* gen_path = DATACRUMBS_SRC_GEN_PATH;
  if (gen_path) {
    std::filesystem::create_directories(std::filesystem::path(gen_path) /
                                        "datacrumbs/server/process");
    std::string generated_process_filename =
        (std::filesystem::path(gen_path) / "datacrumbs/server/process" / "generated_process.h")
            .string();
    std::ofstream generated_process_out(generated_process_filename);
    if (!generated_process_out) {
      DC_LOG_ERROR("Failed to open file for writing: %s", generated_process_filename.c_str());
    } else {
      for (const auto& process_header : process_headers) {
        generated_process_out << "#include \"" << process_header << "\"" << std::endl;
      }
      generated_process_out.close();
      DC_LOG_INFO("[ProbeGenerator] All process headers included in: %s",
                  generated_process_filename.c_str());
    }
  }

  // Clean up JSON object
  json_object_put(probesJson);

  DC_LOG_INFO("[ProbeGenerator] Writing category map...");
  writeCategoryMap();
  // Print total number of probes generated
  DC_LOG_PRINT("[ProbeGenerator] Total number of probes generated: %zu", total_probes_generated);
  DC_LOG_TRACE("[ProbeGenerator] run() completed.");
  return total_probes_generated;
}

// Writes the category map to a JSON file
void ProbeGenerator::writeCategoryMap() {
  const auto& categoryMapFile = configManager_->category_map_path;
  struct json_object* outJson = json_object_new_object();

  // Add each eventId and its info to the JSON object
  for (const auto& [eventId, info] : categoryMap_) {
    char key[32];
    snprintf(key, sizeof(key), "%d", eventId);
    json_object_object_add(outJson, key, json_object_get(info));
  }

  // Write JSON to file
  json_object_to_file_ext(categoryMapFile.c_str(), outJson, JSON_C_TO_STRING_PRETTY);
  json_object_put(outJson);

  // Free info objects
  for (auto& [_, info] : categoryMap_) {
    json_object_put(info);
  }
}

}  // namespace datacrumbs

// Entry point for the generator
int main(int argc, char** argv) {
  datacrumbs::utils::Timer timer;
  timer.resumeTime();

  datacrumbs::ProbeGenerator generator(argc, argv);
  generator.run();

  timer.pauseTime();
  DC_LOG_PRINT("Total time elapsed for Probe Generator: %f seconds", timer.getElapsedTime());

  return 0;
}