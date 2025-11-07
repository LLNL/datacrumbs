// BPF Headers
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
// Generated Headers
#include <datacrumbs/datacrumbs_config.h>
#include <datacrumbs/server/bpf/datacrumbs.skel.h>
#include <datacrumbs/validator/bpf/datacrumbs_validator.skel.h>
// other headers
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/logging.h>  // Logging header
#include <datacrumbs/common/singleton.h>
// std headers
#include <errno.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace datacrumbs {

// Function to pretty print invalid probes categorized by probe name
void prettyPrintInvalidProbes(
    const std::unordered_map<std::string, std::vector<std::string>>& invalid_function_names,
    size_t total_probes, size_t invalid_probes) {
  std::cout << "\n";
  std::cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║                     PROBE VALIDATION SUMMARY                               ║\n";
  std::cout << "╠════════════════════════════════════════════════════════════════════════════╣\n";
  std::cout << "║ Total Probes:   " << std::setw(58) << std::left << total_probes << "║\n";
  std::cout << "║ Invalid Probes: " << std::setw(58) << std::left << invalid_probes << "║\n";
  std::cout << "║ Valid Probes:   " << std::setw(58) << std::left << (total_probes - invalid_probes)
            << "║\n";
  std::cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";

  if (invalid_function_names.empty()) {
    std::cout << "\n✅ All probes validated successfully!\n\n";
    return;
  }

  std::cout << "\n";
  std::cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║                     INVALID PROBES BY CATEGORY                            ║\n";
  std::cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";

  for (const auto& [category, functions] : invalid_function_names) {
    std::cout << "\n┌─ Category: " << category << " (" << functions.size() << " invalid)\n";
    std::cout << "│\n";

    // Show first 10 invalid functions for each category
    size_t max_display = std::min<size_t>(10, functions.size());
    for (size_t i = 0; i < max_display; ++i) {
      std::cout << "│  ✗ " << functions[i] << "\n";
    }

    if (functions.size() > max_display) {
      std::cout << "│  ... and " << (functions.size() - max_display) << " more\n";
    }
    std::cout << "└─────────────────────────────────────────────────────────────────────────────\n";
  }

  std::cout << "\n";
}

// Function to set memory limit to 2GB
bool setMemoryLimit(size_t limit_mb = 2048) {
  struct rlimit limit;
  limit.rlim_cur = limit_mb * 1024 * 1024;  // 2GB in bytes
  limit.rlim_max = limit_mb * 1024 * 1024;  // 2GB in bytes

  if (setrlimit(RLIMIT_AS, &limit) != 0) {
    DC_LOG_ERROR("Failed to set memory limit: %s", strerror(errno));
    return false;
  }

  DC_LOG_INFO("Memory limit set to %zu MB", limit_mb);
  return true;
}

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

    // Use RAII wrapper for automatic cleanup
    auto json_cleanup = [](struct json_object* ptr) {
      if (ptr) {
        json_object_put(ptr);
      }
    };
    std::unique_ptr<struct json_object, decltype(json_cleanup)> json_guard(probesJson,
                                                                           json_cleanup);

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

    auto validate_func = [&](const std::string& probe_name, ProbeType probe_type,
                             const std::string& func, struct json_object* jprobe) {
      atomic_current_probe++;
      DC_LOG_PROGRESS("Validating probe", atomic_current_probe.load(), total_probes);
      bool is_invalid = false;
      if (probe_type == ProbeType::KPROBE) {
        struct bpf_kprobe_opts opts = {
            .sz = sizeof(struct bpf_kprobe_opts),
        };
        struct bpf_link* link = bpf_program__attach_kprobe_opts(kprobe_prog, func.c_str(), &opts);
        if (!link) {
          is_invalid = true;
        } else {
          bpf_link__destroy(link);
        }
      } else if (probe_type == ProbeType::UPROBE) {
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
      } else if (probe_type == ProbeType::SYSCALLS) {
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
        invalid_function_names[probe_name].push_back(func);
      }
    };

    // Use a thread pool of 4 workers
    const size_t num_workers = std::max<size_t>(1, std::thread::hardware_concurrency());
    DC_LOG_INFO("Using %zu worker threads for probe validation", num_workers);
    std::vector<std::thread> workers;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool done = false;

    // Store probe_name, probe_type, function_name, and json_object
    std::queue<std::tuple<std::string, ProbeType, std::string, struct json_object*>> task_queue;

    // Producer: enqueue all validation tasks
    for (int i = 0; i < arr_len; ++i) {
      struct json_object* jprobe = json_object_array_get_idx(probesJson, i);
      auto probe = Probe::fromJson(jprobe);
      for (size_t func_index = 0; func_index < probe.functions.size(); ++func_index) {
        const auto& func = probe.functions[func_index];
        std::lock_guard<std::mutex> lock(queue_mutex);
        // Store copies of probe.name and probe.type to avoid race conditions
        task_queue.emplace(probe.name, probe.type, func, jprobe);
      }
    }

    // Worker threads
    for (size_t i = 0; i < num_workers; ++i) {
      workers.emplace_back([&]() {
        while (true) {
          {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [&]() { return !task_queue.empty() || done; });
            if (task_queue.empty()) {
              if (done)
                break;
              else
                continue;
            }
            // Extract data from the queue with proper copies
            auto& front_task = task_queue.front();
            std::string probe_name = std::get<0>(front_task);
            ProbeType probe_type = std::get<1>(front_task);
            std::string func = std::get<2>(front_task);
            struct json_object* jprobe = std::get<3>(front_task);
            task_queue.pop();

            // Release lock before validation (which can be slow)
            lock.unlock();

            // Now validate with copied data
            validate_func(probe_name, probe_type, func, jprobe);
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

    // Verify counts match - diagnostic check
    size_t total_invalid_stored = 0;
    for (const auto& [name, functions] : invalid_function_names) {
      total_invalid_stored += functions.size();
    }

    if (total_invalid_stored != invalid_probes) {
      DC_LOG_ERROR("Race condition detected! Invalid probes counted: %zu, but stored: %zu",
                   invalid_probes, total_invalid_stored);
    } else {
      DC_LOG_INFO("Validation counts verified: %zu invalid probes counted and stored",
                  invalid_probes);
    }

    return invalid_function_names;
  }
  size_t total_probes() const { return configManager_->category_map.size(); }
  size_t invalid_probes;

 private:
  std::shared_ptr<ConfigurationManager> configManager_;
};
}  // namespace datacrumbs

int main(int argc, char** argv) {
  // Set memory limit to 2GB
  if (!datacrumbs::setMemoryLimit(2048)) {
    DC_LOG_ERROR("Failed to set memory limit, continuing anyway...");
  }

  auto configManager_ =
      datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance(argc, argv);

  struct validator* skel = validator__open_and_load();
  if (!skel) {
    DC_LOG_ERROR("Failed to open and load datacrumbs_validator BPF skeleton");
    return 1;
  }

  // Use RAII for skeleton cleanup
  auto skel_cleanup = [](struct validator* s) {
    if (s) validator__destroy(s);
  };
  std::unique_ptr<struct validator, decltype(skel_cleanup)> skel_guard(skel, skel_cleanup);

  size_t invalid_probes = 0;
  try {
    datacrumbs::ProbeValidator validator(argc, argv);
    auto invalid_function_names = validator.ValidateProbes(skel);
    auto total_probes = validator.total_probes();
    invalid_probes = validator.invalid_probes;
    DC_LOG_INFO("\nProbe validation completed: total_probes=%zu, invalid_probes=%zu", total_probes,
                invalid_probes);

    // Pretty print the invalid probes categorized by category
    datacrumbs::prettyPrintInvalidProbes(invalid_function_names, total_probes, invalid_probes);

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
    return -1;
  }

  return invalid_probes > 0 ? -1 : 0;
}