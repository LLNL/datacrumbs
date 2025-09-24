// BPF Headers
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
// Generated Headers
#include <datacrumbs/bpf/datacrumbs_validator.skel.h>
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/logging.h>  // Logging header
#include <datacrumbs/common/singleton.h>
// std headers
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
namespace datacrumbs {
class ProbeValidator {
 public:
  ProbeValidator(int argc, char** argv) {
    // Initialize config manager (similar to probe explorer)
    configManager_ = Singleton<ConfigurationManager>::get_instance(argc, argv);
  }

  std::unordered_map<std::string, std::vector<std::string>> ValidateProbes(struct validator* skel) {
    bool all_ok = true;
    // Get probes file path from configuration
    const auto& probesFile = configManager_->probe_file_path;
    DC_LOG_INFO("[ProbeGenerator] Reading probes file: %s", probesFile.c_str());

    std::unordered_map<std::string, std::vector<std::string>> invalid_function_names;
    // Read probes JSON file
    struct json_object* probesJson = json_object_from_file(probesFile.c_str());
    if (!probesJson) {
      DC_LOG_ERROR("Failed to read probes file: %s", probesFile.c_str());
      return invalid_function_names;
    }
    int arr_len = json_object_array_length(probesJson);
    DC_LOG_INFO("[ProbeGenerator] Number of probes: %d", arr_len);
    struct bpf_program* kprobe_prog = bpf_object__find_program_by_name(skel->obj, "kprobe_test");
    if (!kprobe_prog) {
      DC_LOG_ERROR("Failed to find kprobe_test program in BPF object");
      return invalid_function_names;
    }
    struct bpf_program* uprobe_prog = bpf_object__find_program_by_name(skel->obj, "uprobe_test");
    if (!uprobe_prog) {
      DC_LOG_ERROR("Failed to find uprobe_test program in BPF object");
      return invalid_function_names;
    }
    struct bpf_program* syscall_prog = bpf_object__find_program_by_name(skel->obj, "syscall_test");
    if (!syscall_prog) {
      DC_LOG_ERROR("Failed to find syscall_test program in BPF object");
      return invalid_function_names;
    }
    size_t total_probes = configManager_->category_map.size();
    size_t current_probe = 0;
    invalid_probes = 0;

    std::mutex invalid_mutex;
    std::atomic<size_t> atomic_invalid_probes{0};
    std::atomic<size_t> atomic_current_probe{0};

    auto validate_func = [&](const Probe& probe, const std::string& func,
                             struct json_object* jprobe) {
      atomic_current_probe++;
      DC_LOG_PROGRESS("Validating probe", atomic_current_probe.load(), total_probes);
      bool is_invalid = false;
      if (probe.type == ProbeType::KPROBE) {
        struct bpf_kprobe_opts opts = {
            .sz = sizeof(struct bpf_kprobe_opts),
        };
        struct bpf_link* link = bpf_program__attach_kprobe_opts(kprobe_prog, func.c_str(), &opts);
        if (!link) {
          is_invalid = true;
        } else {
          bpf_link__destroy(link);
        }
      } else if (probe.type == ProbeType::UPROBE) {
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
        unsigned long offset_val = 0;
        if (!offset.empty()) {
          offset_val = std::stoul(offset.c_str(), nullptr, 0);
        }
        struct bpf_link* link = bpf_program__attach_uprobe_opts(
            uprobe_prog, -1, uprobe.binary_path.c_str(), offset_val, nullptr);
        if (!link) {
          is_invalid = true;
        } else {
          bpf_link__destroy(link);
        }
      } else if (probe.type == ProbeType::SYSCALLS) {
        struct bpf_ksyscall_opts opts = {
            .sz = sizeof(struct bpf_ksyscall_opts),
        };
        struct bpf_link* link = bpf_program__attach_ksyscall(syscall_prog, func.c_str(), &opts);
        if (!link) {
          is_invalid = true;
        } else {
          bpf_link__destroy(link);
        }
      }
      if (is_invalid) {
        atomic_invalid_probes++;
        std::lock_guard<std::mutex> lock(invalid_mutex);
        invalid_function_names[probe.name].push_back(func);
      }
    };

    // Use a thread pool of 4 workers
    const size_t num_workers = std::max<size_t>(1, std::thread::hardware_concurrency());
    DC_LOG_INFO("Using %zu worker threads for probe validation", num_workers);
    std::vector<std::thread> workers;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool done = false;
    std::queue<std::tuple<Probe, std::string, struct json_object*>> task_queue;

    // Producer: enqueue all validation tasks
    for (int i = 0; i < arr_len; ++i) {
      struct json_object* jprobe = json_object_array_get_idx(probesJson, i);
      auto probe = Probe::fromJson(jprobe);
      for (size_t func_index = 0; func_index < probe.functions.size(); ++func_index) {
        const auto& func = probe.functions[func_index];
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.emplace(probe, func, jprobe);
      }
    }

    // Worker threads
    for (size_t i = 0; i < num_workers; ++i) {
      workers.emplace_back([&]() {
        while (true) {
          std::tuple<Probe, std::string, struct json_object*> task;
          {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [&]() { return !task_queue.empty() || done; });
            if (task_queue.empty()) {
              if (done)
                break;
              else
                continue;
            }
            // Move declaration and initialization together to avoid default construction
            auto task = std::move(task_queue.front());
            task_queue.pop();
            validate_func(std::get<0>(task), std::get<1>(task), std::get<2>(task));
          }
        }
      });
    }

    // Notify workers after all tasks are enqueued
    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      done = true;
    }
    cv.notify_all();

    for (auto& t : workers) t.join();

    invalid_probes = atomic_invalid_probes.load();
    return invalid_function_names;
  }
  size_t total_probes() const { return configManager_->category_map.size(); }
  size_t invalid_probes;

 private:
  std::shared_ptr<ConfigurationManager> configManager_;
};
}  // namespace datacrumbs

int main(int argc, char** argv) {
  auto configManager_ =
      datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance(argc, argv);
  struct validator* skel = validator__open_and_load();
  if (!skel) {
    DC_LOG_ERROR("Failed to open and load datacrumbs_validator BPF skeleton");
    return 1;
  }
  size_t invalid_probes = 0;
  try {
    datacrumbs::ProbeValidator validator(argc, argv);
    auto invalid_function_names = validator.ValidateProbes(skel);
    auto total_probes = validator.total_probes();
    invalid_probes = validator.invalid_probes;
    DC_LOG_INFO("\nProbe validation completed: total_probes=%zu, invalid_probes=%zu", total_probes,
                invalid_probes);
    struct json_object* invalid_probesJson =
        json_object_from_file(configManager_->probe_file_path.c_str());
    // Iterate over probesJson array, clear functions attribute for all probes, then add invalid
    // functions if any
    int arr_len = json_object_array_length(invalid_probesJson);
    for (int i = 0; i < arr_len; ++i) {
      struct json_object* jprobe = json_object_array_get_idx(invalid_probesJson, i);
      // Get probe name
      struct json_object* jname = nullptr;
      if (!json_object_object_get_ex(jprobe, "name", &jname)) continue;
      std::string probe_name = json_object_get_string(jname);

      // Always clear the "functions" array
      struct json_object* jfunctions = json_object_new_array();
      json_object_object_add(jprobe, "functions", jfunctions);

      // If this probe has invalid functions, add them to the "functions" attribute
      auto it = invalid_function_names.find(probe_name);
      if (it != invalid_function_names.end()) {
        for (const auto& func : it->second) {
          json_object_array_add(jfunctions, json_object_new_string(func.c_str()));
        }
      }
    }
    // Write the updated invalid_probesJson to the probe_invalid_file_path
    const auto& invalidFile = configManager_->probe_invalid_file_path;
    if (json_object_to_file_ext(invalidFile.c_str(), invalid_probesJson, JSON_C_TO_STRING_PRETTY) !=
        0) {
      DC_LOG_ERROR("Failed to write invalid probes JSON to file: %s", invalidFile.c_str());
    } else {
      DC_LOG_INFO("Invalid probes JSON written to: %s", invalidFile.c_str());
    }
    json_object_put(invalid_probesJson);  // free JSON object
    // Set ownership and permissions for the invalid probes file
    auto pwd = getpwnam(configManager_->user.c_str());
    uid_t uid = pwd ? pwd->pw_uid : static_cast<uid_t>(-1);
    gid_t gid = pwd ? pwd->pw_gid : static_cast<gid_t>(-1);
    // Set file ownership to configManager_->user
    chown(configManager_->probe_invalid_file_path.c_str(), uid, gid);
    // Optionally set permissions (e.g., rw-r-----)
    chmod(configManager_->probe_invalid_file_path.c_str(), 0640);
  } catch (const std::exception& ex) {
    DC_LOG_ERROR("Exception: %s", ex.what());
    validator__destroy(skel);
    return -1;
  }

  validator__destroy(skel);
  return invalid_probes > 0 ? -1 : 0;
}