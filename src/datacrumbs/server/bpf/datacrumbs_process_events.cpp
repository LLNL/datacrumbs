#include <bpf/libbpf.h>
#include <datacrumbs/bpf/datacrumbs.skel.h>
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/server/bpf/shared.h>
#include <errno.h>
#include <grp.h>
#include <json-c/json.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

struct context {
  std::unordered_map<uint64_t, std::pair<std::string, std::string>>* category_map;
  std::shared_ptr<datacrumbs::ConfigurationManager> config_manager;
};

static int libbpf_print_fn(enum libbpf_print_level level, const char* format, va_list args) {
  if (level >= LIBBPF_DEBUG) return 0;
  return vfprintf(stderr, format, args);
}
int handle_event(void* ctx, void* data, size_t data_sz) {
  context* ctx_ptr = static_cast<context*>(ctx);
  auto* category_map = ctx_ptr->category_map;
  struct general_event_t* m = (general_event_t*)data;
  unsigned int pid = m->id;

  if (pid == 0) {
    return 0;
  }
  auto it = category_map->find(m->event_id);
  if (it != category_map->end()) {
    const auto& [probe_name, function_name] = it->second;
    if (!probe_name.empty() && !function_name.empty()) {
      printf("%-6u  %-6llu %-6llu %-6llu  %s.%s\n", pid, m->event_id, m->ts, m->dur,
             probe_name.c_str(), function_name.c_str());
    } else if (!probe_name.empty()) {
      printf("%-6u  %-6llu %-6llu %-6llu  %s\n", pid, m->event_id, m->ts, m->dur,
             probe_name.c_str());
    } else if (!function_name.empty()) {
      printf("%-6u  %-6llu %-6llu %-6llu  %s\n", pid, m->event_id, m->ts, m->dur,
             function_name.c_str());
    } else {
      printf("%-6u  %-6llu %-6llu %-6llu\n", pid, m->event_id, m->ts, m->dur);
    }
    static size_t event_index = 0;
    static FILE* trace_file = nullptr;
    static char trace_buffer[32 * 1024 * 1024];  // 32 MB buffer

    if (!trace_file) {
      trace_file = fopen(ctx_ptr->config_manager->trace_file_path.c_str(), "a");
      if (trace_file) {
        struct passwd* pwd = getpwnam(ctx_ptr->config_manager->user.c_str());
        uid_t uid = pwd ? pwd->pw_uid : -1;
        gid_t gid = pwd ? pwd->pw_gid : -1;
        // Set file ownership to configManager->user
        chown(ctx_ptr->config_manager->trace_file_path.c_str(), uid, gid);
        // Optionally set permissions (e.g., rw-r-----)
        chmod(ctx_ptr->config_manager->trace_file_path.c_str(), 0640);
      }
      if (trace_file) {
        setvbuf(trace_file, trace_buffer, _IOLBF, sizeof(trace_buffer));
      }
    }
    if (trace_file) {
      if (event_index == 0) {
        fprintf(trace_file, "[\n");  // Start of JSON array
      }
      fprintf(trace_file,
              "{\"id\":%zu,\"pid\":%u,\"tid\":%u,\"cat\":\"%s\",\"name\":\"%s\",\"ts\":%llu,"
              "\"dur\":%llu}\n",
              event_index++, pid, pid, probe_name.c_str(), function_name.c_str(),
              (unsigned long long)m->ts, (unsigned long long)(m->dur / 1000));
      // fflush(trace_file); // flush on every new line
    }
  } else {
    // If no category found, print without category information
    // This is useful for debugging or when the event_id is not in the category map
    printf("No category found for event_id %llu\n", m->event_id);
  }
  return 0;
}
void lost_event(void* ctx, int cpu, long long unsigned int data_sz) {
  printf("lost event\n");
}
int main(int argc, char** argv) {
  struct datacrumbs_bpf* skel;
  int err;
  struct ring_buffer* rb = NULL;
  libbpf_set_print(libbpf_print_fn);
  skel = datacrumbs_bpf__open_and_load();
  if (!skel) {
    printf("Failed to open BPF object\n");
    return 1;
  }

  auto configManager_ =
      datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance(argc, argv);
  if (!configManager_) {
    fprintf(stderr, "Failed to get configuration manager instance\n");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  // Load category_map from JSON file using json-c

  std::unordered_map<uint64_t, std::pair<std::string, std::string>> category_map;
  std::string category_json_path = configManager_->category_map_path.string();
  if (category_json_path.empty() || !std::filesystem::exists(category_json_path)) {
    fprintf(stderr, "Category map file does not exist: %s\n", category_json_path.c_str());
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  // Read file into string
  std::ifstream file(category_json_path);
  if (!file) {
    fprintf(stderr, "Failed to open category map file: %s\n", category_json_path.c_str());
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  // Parse JSON
  struct json_object* root = json_tokener_parse(json_str.c_str());
  if (!root) {
    fprintf(stderr, "Failed to parse JSON from %s\n", category_json_path.c_str());
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  // Expecting a JSON object with event_id as keys
  json_object_object_foreach(root, key, val) {
    uint64_t event_id = std::stoull(key);
    const char* probe_name = nullptr;
    const char* function_name = nullptr;

    struct json_object* probe_obj = nullptr;
    struct json_object* func_obj = nullptr;

    if (json_object_object_get_ex(val, "probe_name", &probe_obj) &&
        json_object_object_get_ex(val, "function_name", &func_obj)) {
      probe_name = json_object_get_string(probe_obj);
      function_name = json_object_get_string(func_obj);
      category_map[event_id] =
          std::make_pair(probe_name ? probe_name : "", function_name ? function_name : "");
    }
  }
  json_object_put(root);
  err = datacrumbs_bpf__attach(skel);
  if (err) {
    fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  context ctx;
  ctx.category_map = &category_map;
  ctx.config_manager = configManager_;
  rb = ring_buffer__new(bpf_map__fd(skel->maps.output), handle_event, &ctx, NULL);
  if (!rb) {
    err = -1;
    fprintf(stderr, "Failed to create ring buffer\n");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  printf("%-6s  %-6s %-6s %-6s\n", "PID", "EID", "TS", "DUR");
  printf("Ready to run the code.\n");
  while (true) {
    err = ring_buffer__poll(rb, 30000);
    // Ctrl-C gives -EINTR
    if (err == -EINTR) {
      err = 0;
      break;
    }
    if (err < 0) {
      printf("Error polling ring buffer: %d\n", err);
      break;
    }
  }
  ring_buffer__free(rb);
  datacrumbs_bpf__destroy(skel);
  return -err;
}