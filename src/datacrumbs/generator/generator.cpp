#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/generator/generator.h>
namespace datacrumbs {
ProbeGenerator::ProbeGenerator(int argc, char** argv) {
  configManager_ = datacrumbs::Singleton<ConfigurationManager>::get_instance(argc, argv);
}

void ProbeGenerator::run() {
  std::cout << "[ProbeGenerator] Starting run()" << std::endl;
  if (!configManager_) {
    std::cerr << "ConfigurationManager is not initialized." << std::endl;
    return;
  }

  const auto& probesFile = configManager_->probe_file_path;
  std::cout << "[ProbeGenerator] Reading probes file: " << probesFile << std::endl;
  struct json_object* probesJson = json_object_from_file(probesFile.c_str());
  if (!probesJson) {
    std::cerr << "Failed to read probes file: " << probesFile << std::endl;
    return;
  }
  auto probe_files = std::vector<std::string>();
  int arr_len = json_object_array_length(probesJson);
  std::cout << "[ProbeGenerator] Number of probes: " << arr_len << std::endl;
  for (int i = 0; i < arr_len; ++i) {
    struct json_object* jprobe = json_object_array_get_idx(probesJson, i);
    auto probe = Probe::fromJson(jprobe);
    std::cout << "[ProbeGenerator] Processing probe: " << probe.name << std::endl;
    std::stringstream ss;
    ss << "#include <datacrumbs/server/bpf/common.h>" << std::endl;
    for (const auto& func : probe.functions) {
      auto current_event_id = this->eventIdCounter_++;
      std::cout << "[ProbeGenerator] Generating for function: " << func
                << " (event_id: " << current_event_id << ")" << std::endl;
      // Map event id to probe name and function name
      struct json_object* info = json_object_new_object();
      json_object_object_add(info, "probe_name", json_object_new_string(probe.name.c_str()));
      json_object_object_add(info, "function_name", json_object_new_string(func.c_str()));
      categoryMap_[current_event_id] = info;

      switch (probe.type) {
        case ProbeType::KPROBE: {
          std::cout << "[ProbeGenerator] Using KProbeGenerator" << std::endl;
          KProbeGenerator generator(current_event_id, func);
          ss << generator.generate().str() << std::endl;
          break;
        }
        case ProbeType::UPROBE: {
          std::cout << "[ProbeGenerator] Using UProbeGenerator" << std::endl;
          auto uprobe = UProbe::fromJson(jprobe);
          UProbeGenerator uprobe_gen(current_event_id, func, uprobe.binary_path);
          ss << uprobe_gen.generate().str() << std::endl;
          break;
        }
        case ProbeType::SYSCALLS: {
          std::cout << "[ProbeGenerator] Using SyscallGenerator" << std::endl;
          SyscallGenerator syscall_gen(current_event_id, func);
          ss << syscall_gen.generate().str() << std::endl;
          break;
        }
        case ProbeType::USDT: {
          std::cout << "[ProbeGenerator] Using USDTGenerator" << std::endl;
          auto usdt = USDTProbe::fromJson(jprobe);
          USDTGenerator usdt_gen(current_event_id, func, usdt.binary_path, usdt.provider);
          ss << usdt_gen.generate().str() << std::endl;
        } break;
        default: {
          std::cerr << "Unknown probe type: " << static_cast<int>(probe.type) << std::endl;
        }
      }
    }
    const char* gen_path = DATACRUMBS_SRC_GEN_PATH;
    if (!gen_path) {
      std::cerr << "DATACRUMBS_SRC_GEN_PATH environment variable not set." << std::endl;
    } else {
      std::filesystem::create_directories(gen_path / std::filesystem::path("server/bpf"));
      std::string filename =
          (gen_path / std::filesystem::path("datacrumbs/server/bpf") / (probe.name + ".bpf.c"))
              .string();
      std::ofstream out(filename);
      if (!out) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
      } else {
        out << ss.str();
        out.close();
        probe_files.push_back(probe.name + ".bpf.c");
        std::cout << "[ProbeGenerator] Generated file: " << filename << std::endl;
      }
    }
  }
  // Append all generated probe files as includes to generated.bpf.c
  const char* gen_path = DATACRUMBS_SRC_GEN_PATH;
  if (gen_path) {
    std::string generated_filename =
        (gen_path / std::filesystem::path("datacrumbs/server/bpf") / ("generated.bpf.c")).string();
    std::ofstream generated_out(generated_filename);
    if (!generated_out) {
      std::cerr << "Failed to open file for writing: " << generated_filename << std::endl;
    } else {
      for (const auto& probe_file : probe_files) {
        generated_out << "#include \"" << probe_file << "\"" << std::endl;
      }
      generated_out.close();
      std::cout << "[ProbeGenerator] All probe files included in: " << generated_filename
                << std::endl;
    }
  }
  json_object_put(probesJson);
  std::cout << "[ProbeGenerator] Writing category map..." << std::endl;
  writeCategoryMap();
  std::cout << "[ProbeGenerator] run() completed." << std::endl;
}

void ProbeGenerator::writeCategoryMap() {
  const auto& categoryMapFile = configManager_->category_map_path;
  struct json_object* outJson = json_object_new_object();
  for (const auto& [eventId, info] : categoryMap_) {
    char key[32];
    snprintf(key, sizeof(key), "%d", eventId);
    json_object_object_add(outJson, key, json_object_get(info));
  }
  json_object_to_file_ext(categoryMapFile.c_str(), outJson, JSON_C_TO_STRING_PRETTY);
  json_object_put(outJson);
  // Free info objects
  for (auto& [_, info] : categoryMap_) {
    json_object_put(info);
  }
}
}  // namespace datacrumbs
int main(int argc, char** argv) {
  datacrumbs::ProbeGenerator generator(argc, argv);
  generator.run();
  return 0;
}