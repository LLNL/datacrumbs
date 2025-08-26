#include <datacrumbs/server/process/event_processor.h>

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 2)

#define INITIALIZE_MAP_1()                                                  \
  int profile_1_fd = bpf_map__fd(skel->maps.profile);                       \
  if (profile_1_fd < 0) {                                                   \
    DC_LOG_ERROR("Failed to get general profile map fd: %d", profile_1_fd); \
    datacrumbs_bpf__destroy(skel);                                          \
    return 1;                                                               \
  }

#define INITIALIZE_MAP_LOOKUP_1()                                                   \
  struct profile_key_t* profile_keys =                                              \
      (struct profile_key_t*)malloc(batch_size * sizeof(struct profile_key_t));     \
  struct profile_value_t* profile_values =                                          \
      (struct profile_value_t*)malloc(batch_size * sizeof(struct profile_value_t)); \
  struct profile_key_t* profile_in_batch = nullptr;

#define LOOKUP_1_CALL()                                                                         \
  lookup_1(profile_1_fd, latest_ts, &event_processor, batch_size, profile_keys, profile_values, \
           profile_in_batch)

inline static int lookup_1(int map_fd, unsigned long long latest_timestamp,
                           datacrumbs::EventProcessor* event_processor, unsigned int batch_size,
                           struct profile_key_t* keys, struct profile_value_t* values,
                           struct profile_key_t* in_batch) {
  int ret = bpf_map_lookup_batch(map_fd, in_batch, &in_batch, keys, values, &batch_size, 0);
  if (ret < 0 && errno != ENOENT) {
    perror("bpf_map_lookup_batch general");
    return -1;
  }
  if (batch_size < 1) {
    return -1;
  }
  struct profile_key_t delete_keys[batch_size];
  unsigned int j = 0;
  // Process the retrieved keys and values
  for (int i = 0; i < batch_size; ++i) {
    if (latest_timestamp == 0 || keys[i].time_interval <= latest_timestamp) {
      struct counter_event_t event;
      event.key = &keys[i];
      event.value = &values[i];
      event_processor->handle_event(&event, 1024);
      delete_keys[j++] = keys[i];
    }
  }
  ret = bpf_map_delete_batch(map_fd, delete_keys, &j, NULL);
  if (ret < 0) {
    perror("bpf_map_delete_batch general");
  }
  // Check if the end of the map has been reached
  if (ret < 0 && errno == ENOENT) {
    return -1;
  }
  return 0;
}

#endif
static datacrumbs::EventWithId* get_data_1(void* data, uint64_t index) {
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  struct general_event_t* base = (general_event_t*)data;

  auto event = new datacrumbs::EventWithId(NORMAL_EVENT, index, base->type, base->id,
                                           base->event_id, base->ts, base->dur, nullptr);
#else
  struct counter_event_t* base = (counter_event_t*)data;
  auto args = new DataCrumbsArgs();
  args->emplace("duration", base->value->duration);
  args->emplace("frequency", base->value->frequency);
  auto event = new datacrumbs::EventWithId(COUNTER_EVENT, index, base->key->type, base->key->id,
                                           base->key->event_id, base->key->time_interval, 0, args);
#endif
  return event;
}

#define GET_DATA_3_EXISTS

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 2)
#define INITIALIZE_MAP_3()                                          \
  int profile_3_fd = bpf_map__fd(skel->maps.usdt_profile);          \
  if (profile_3_fd < 0) {                                           \
    DC_LOG_ERROR("Failed to get profile map fd: %d", profile_3_fd); \
    datacrumbs_bpf__destroy(skel);                                  \
    return 1;                                                       \
  }

#define INITIALIZE_MAP_LOOKUP_3()                                                         \
  struct usdt_profile_key_t* usdt_keys =                                                  \
      (struct usdt_profile_key_t*)malloc(batch_size * sizeof(struct usdt_profile_key_t)); \
  struct profile_value_t* usdt_values =                                                   \
      (struct profile_value_t*)malloc(batch_size * sizeof(struct profile_value_t));       \
  struct usdt_profile_key_t* usdt_in_batch = nullptr;

#define LOOKUP_3_CALL()                                                                   \
  lookup_3(profile_3_fd, latest_ts, &event_processor, batch_size, usdt_keys, usdt_values, \
           usdt_in_batch)

inline static int lookup_3(int map_fd, unsigned long long latest_timestamp,
                           datacrumbs::EventProcessor* event_processor, unsigned int batch_size,
                           struct usdt_profile_key_t* keys, struct profile_value_t* values,
                           struct usdt_profile_key_t* in_batch) {
  int ret = bpf_map_lookup_batch(map_fd, in_batch, &in_batch, keys, values, &batch_size, 0);
  if (ret < 0 && errno != ENOENT) {
    perror("bpf_map_lookup_batch usdt");
    return -1;
  }
  if (batch_size < 1) {
    return -1;
  }
  struct usdt_profile_key_t delete_keys[batch_size];
  unsigned int j = 0;
  // Process the retrieved keys and values
  for (int i = 0; i < batch_size; ++i) {
    if (latest_timestamp == 0 || keys[i].time_interval <= latest_timestamp) {
      struct usdt_counter_event_t event;
      event.key = &keys[i];
      event.value = &values[i];
      event_processor->handle_event(&event, 1024);
      delete_keys[j++] = keys[i];
    }
  }
  ret = bpf_map_delete_batch(map_fd, delete_keys, &j, NULL);
  if (ret < 0) {
    perror("bpf_map_delete_batch usdt");
  }
  // Check if the end of the map has been reached
  if (ret < 0 && errno == ENOENT) {
    return -1;
  }
  return 0;
}
#endif

static datacrumbs::EventWithId* get_data_3(void* data, uint64_t index) {
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  struct usdt_event_t* base = (usdt_event_t*)data;
  auto args = new DataCrumbsArgs();
  args->emplace("clazz", base->class_hash);
  args->emplace("method", base->method_hash);
  auto event = new datacrumbs::EventWithId(NORMAL_EVENT, index, base->type, base->id,
                                           base->event_id, base->ts, base->dur, args);
#else
  struct usdt_counter_event_t* base = (usdt_counter_event_t*)data;
  auto args = new DataCrumbsArgs();
  args->emplace("duration", base->value->duration);
  args->emplace("frequency", base->value->frequency);
  args->emplace("clazz", base->key->class_hash);
  args->emplace("method", base->key->method_hash);
  auto event = new datacrumbs::EventWithId(COUNTER_EVENT, index, base->key->type, base->key->id,
                                           base->key->event_id, base->key->time_interval, 0, args);
#endif
  return event;
}

#define GET_DATA_FUNCTION(INDEX)                                       \
  auto write_event = get_data_##INDEX(data, event_index.fetch_add(1)); \
  if (write_event != nullptr) {                                        \
    writer->push_event(write_event);                                   \
  }

namespace datacrumbs {
EventProcessor::EventProcessor(int argc, char** argv) {
  configManager_ =
      datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance(argc, argv);
  // Initialize the ChromeWriter singleton instance
  writer_ = datacrumbs::Singleton<datacrumbs::ChromeWriter>::get_instance();
  if (!writer_) {
    DC_LOG_ERROR("Failed to create ChromeWriter instance");
  }
}
int EventProcessor::handle_event(void* data, size_t data_sz) {
  DC_LOG_TRACE("handle_event: start");

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  struct general_event_t* event = (general_event_t*)data;
#else
  struct profile_key_t* event = (profile_key_t*)((counter_event_t*)data)->key;
#endif
  unsigned int pid = event->id;

  if (pid == 0) {
    DC_LOG_DEBUG("handle_event: pid is 0, skipping event");
    return 0;
  }
  auto it = configManager_->category_map.find(event->event_id);
  if (it != configManager_->category_map.end()) {
    const auto& [probe_name, function_name] = it->second;
    // Print event info to stdout for debugging
    DC_LOG_DEBUG("%-6u  %-6llu  %s.%s", pid, event->event_id, probe_name.c_str(),
                 function_name.c_str());
    // Write event to Chrome trace file
    auto writer = datacrumbs::Singleton<datacrumbs::ChromeWriter>::get_instance();
    if (!writer) {
      DC_LOG_ERROR("Failed to create ChromeWriter instance");
      return 1;
    }
    if (event->type > 0) {
      if (event->type == 1) {
        GET_DATA_FUNCTION(1);
      }
#ifdef GET_DATA_2_EXISTS
      else if (event->type == 2) {
        GET_DATA_FUNCTION(2);
      }
#endif
#ifdef GET_DATA_3_EXISTS
      else if (event->type == 3) {
        GET_DATA_FUNCTION(3);
      }
#endif
#ifdef GET_DATA_4_EXISTS
      else if (event->type == 4) {
        GET_DATA_FUNCTION(4);
      }
#endif
#ifdef GET_DATA_5_EXISTS
      else if (event->type == 5) {
        GET_DATA_FUNCTION(5);
      }
#endif
#ifdef GET_DATA_6_EXISTS
      else if (event->type == 6) {
        GET_DATA_FUNCTION(6);
      }
#endif
#ifdef GET_DATA_7_EXISTS
      else if (event->type == 7) {
        GET_DATA_FUNCTION(7);
      }
#endif
#ifdef GET_DATA_8_EXISTS
      else if (event->type == 8) {
        GET_DATA_FUNCTION(8);
      }
#endif
#ifdef GET_DATA_9_EXISTS
      else if (event->type == 9) {
        GET_DATA_FUNCTION(9);
      }
#endif
#ifdef GET_DATA_10_EXISTS
      else if (event->type == 10) {
        GET_DATA_FUNCTION(10);
      }
#endif
      else {
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
        return 0;
      }
    } else {
      DC_LOG_WARN("Event type is not positive, skipping event");
      return 0;
    }

  } else {
    // If no category found, print warning
    DC_LOG_WARN("No category found for event_id %llu", event->event_id);
  }
  DC_LOG_TRACE("handle_event: end");
  DC_LOG_PROGRESS_SINGLE("Processed event", event_index);
  return 0;
}
int EventProcessor::update_filename(const char* filename, unsigned int hash) {
  if (processed_hashes_.find(hash) != processed_hashes_.end()) {
    DC_LOG_DEBUG("Filename %s with hash %u already processed, skipping", filename, hash);
    return 0;  // Skip if already processed
  }
  processed_hashes_.insert(hash);  // Mark this hash as processed
  auto args = new DataCrumbsArgs();
  args->emplace("value", std::string(filename));
  args->emplace("hash", hash);
  auto event =
      new datacrumbs::EventWithId(METADATA_EVENT, event_index.fetch_add(1), 0, 0, 0, 0, 0, args);
  if (writer_) {
    writer_->write_event(event);
  }
  return 0;
}

};  // namespace datacrumbs

// Custom libbpf print function for debugging
static int libbpf_print_fn(enum libbpf_print_level level, const char* format, va_list args) {
  if (level >= LIBBPF_DEBUG) return 0;
  return vfprintf(stderr, format, args);
}

static int handle_event(void* ctx, void* data, size_t data_sz) {
  datacrumbs::EventProcessor* event_processor = static_cast<datacrumbs::EventProcessor*>(ctx);
  return event_processor->handle_event(data, data_sz);
}

inline static int lookup_and_delete(int map_fd, datacrumbs::EventProcessor* event_processor,
                                    struct string_t* keys, unsigned int* values,
                                    unsigned int batch_size, struct string_t* in_batch) {
  int ret =
      bpf_map_lookup_and_delete_batch(map_fd, in_batch, &in_batch, keys, values, &batch_size, 0);
  if (ret < 0 && errno != ENOENT) {
    perror("bpf_map_lookup_and_delete_batch fhash");
    return -1;
  }
  // Process the retrieved keys and values
  for (int i = 0; i < batch_size; ++i) {
    event_processor->update_filename(keys[i].str, values[i]);
  }
  // Check if the end of the map has been reached
  if (ret < 0 && errno == ENOENT) {
    return -1;
  }
  return 0;
}

inline static int lookup(int map_fd, datacrumbs::EventProcessor* event_processor,
                         struct string_t* keys, unsigned int* values, unsigned int batch_size,
                         struct string_t* in_batch) {
  int ret = bpf_map_lookup_batch(map_fd, in_batch, &in_batch, keys, values, &batch_size, 0);
  if (ret < 0 && errno != ENOENT) {
    perror("bpf_map_lookup_batch  fhash");
    return -1;
  }
  // Process the retrieved keys and values
  for (int i = 0; i < batch_size; ++i) {
    event_processor->update_filename(keys[i].str, values[i]);
  }
  // Check if the end of the map has been reached
  if (ret < 0 && errno == ENOENT) {
    return -1;
  }
  return 0;
}

int main(int argc, char** argv) {
  DC_LOG_TRACE("main: start");
  datacrumbs::utils::Timer timer;
  timer.resumeTime();

  struct datacrumbs_bpf* skel;
  int err;
  struct ring_buffer* rb = NULL;
  libbpf_set_print(libbpf_print_fn);

  // Open and load BPF skeleton
  skel = datacrumbs_bpf__open_and_load();
  if (!skel) {
    DC_LOG_ERROR("Failed to open BPF object");
    return 1;
  }

  // Get configuration manager singleton instance
  auto event_processor = datacrumbs::EventProcessor(argc, argv);

  if (!event_processor.configManager_) {
    DC_LOG_ERROR("ConfigurationManager is not initialized");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  if (!event_processor.writer_) {
    DC_LOG_ERROR("Failed to create ChromeWriter instance");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  // Attach BPF skeleton
  err = datacrumbs_bpf__attach(skel);
  if (err) {
    DC_LOG_ERROR("Failed to attach BPF skeleton: %d", err);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  auto config_manager = event_processor.configManager_;
  struct json_object* manual_probes_json =
      json_object_from_file(config_manager->manual_probe_path.c_str());
  if (manual_probes_json) {
    int total_manual_probes = 0, total_successful_probes = 0, total_failed_probes = 0;
    int arr_len = json_object_array_length(manual_probes_json);
    for (int i = 0; i < arr_len; i++) {
      struct json_object* jprobe = json_object_array_get_idx(manual_probes_json, i);
      if (jprobe) {
        auto probe = datacrumbs::Probe::fromJson(jprobe);
        std::shared_ptr<datacrumbs::Probe> manual_probe;
        switch (probe.type) {
          case datacrumbs::ProbeType::UPROBE:
            manual_probe =
                std::make_shared<datacrumbs::UProbe>(datacrumbs::UProbe::fromJson(jprobe));
            break;
          case datacrumbs::ProbeType::SYSCALLS:
            manual_probe = std::make_shared<datacrumbs::SysCallProbe>(
                datacrumbs::SysCallProbe::fromJson(jprobe));
            break;
          case datacrumbs::ProbeType::USDT:
            manual_probe =
                std::make_shared<datacrumbs::USDTProbe>(datacrumbs::USDTProbe::fromJson(jprobe));
            break;
          case datacrumbs::ProbeType::KPROBE:
            manual_probe =
                std::make_shared<datacrumbs::KProbe>(datacrumbs::KProbe::fromJson(jprobe));
            break;
          case datacrumbs::ProbeType::CUSTOM:
            manual_probe = std::make_shared<datacrumbs::CustomProbe>(
                datacrumbs::CustomProbe::fromJson(jprobe));
            break;
          default:
            DC_LOG_ERROR("Unknown probe type encountered in extractProbes()");
            throw std::runtime_error("Unknown probe type encountered in extractProbes()");
        }
        for (const auto& func : manual_probe->functions) {
          total_manual_probes += 2;
          if (probe.type == datacrumbs::ProbeType::UPROBE) {
            auto uprobe = std::dynamic_pointer_cast<datacrumbs::UProbe>(manual_probe);
            uint64_t func_hash = std::stoull(func, nullptr, 0);
            auto func_name_ = config_manager->category_map[func_hash].second;
            DC_LOG_DEBUG("Extracted function name: %s", func_name_.c_str());
            auto pos = func_name_.find(':');
            std::string offset = "";
            bool is_manual = false;
            if (pos != std::string::npos) {
              offset = func_name_.substr(pos + 1);
              func_name_ = func_name_.substr(0, pos);
              is_manual = true;
            } else {
              offset = "";
            }
            if (is_manual) {
              std::string sanitized_func_name = func_name_;
              if (sanitized_func_name.length() > 10) {
                sanitized_func_name = sanitized_func_name.substr(0, 10);
              }
              std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
              std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');
              sanitized_func_name += func;
              auto entry_func = sanitized_func_name + "_entry";
              auto exit_func = sanitized_func_name + "_exit";
              struct bpf_link* link;
              struct bpf_program* prog =
                  bpf_object__find_program_by_name(skel->obj, entry_func.c_str());
              struct bpf_uprobe_opts opts = {
                  .sz = sizeof(struct bpf_uprobe_opts),
              };

              // Convert offset string to hex value if present
              unsigned long offset_val = 0;
              if (offset != "" && prog != NULL) {
                offset_val =
                    std::stoul(offset.c_str(), nullptr, 0);  // Accepts hex ("0x...") or decimal
                DC_LOG_DEBUG("Attaching program %s to %s at offset:0x%lx manually",
                             entry_func.c_str(), uprobe->binary_path.c_str(), offset_val);
                link = bpf_program__attach_uprobe_opts(
                    prog, -1 /* not a retprobe */, uprobe->binary_path.c_str(), offset_val, &opts);

                if (link == NULL) {
                  // This will provide a more specific error code than the skeleton attach.
                  DC_LOG_DEBUG("Failed to attach uprobe %s on offset:0x%lx manually: %d",
                               func_name_.c_str(), offset_val, errno);
                  total_failed_probes += 2;
                } else {
                  total_successful_probes++;
                  // Successfully attached uprobe
                  DC_LOG_DEBUG("Successfully attached uprobe %s on offset:0x%lx manually",
                               func_name_.c_str(), offset_val);
                  opts.retprobe = true;
                  struct bpf_program* prog2 =
                      bpf_object__find_program_by_name(skel->obj, exit_func.c_str());
                  link = bpf_program__attach_uprobe_opts(prog2, -1 /* not a retprobe */,
                                                         uprobe->binary_path.c_str(), offset_val,
                                                         &opts);

                  if (link == NULL) {
                    // This will provide a more specific error code than the skeleton attach.
                    DC_LOG_DEBUG("Failed to attach uretprobe %s on offset:0x%lx manually: %d",
                                 func_name_.c_str(), offset_val, errno);
                    total_failed_probes++;
                  } else {
                    // Successfully attached uretprobe
                    DC_LOG_DEBUG("Successfully attached probe %s on offset:0x%lx manually",
                                 func_name_.c_str(), offset_val);
                    total_successful_probes++;
                  }
                }
              } else {
                DC_LOG_DEBUG("Failed to attach uprobe %s on offset:0x%lx manually: %d",
                             func_name_.c_str(), offset_val, errno);
                total_failed_probes += 2;
              }
            } else {
              DC_LOG_DEBUG("Failed to attach uprobe %s as no offset present", func_name_.c_str());
              total_failed_probes += 2;
            }
          } else {
            DC_LOG_DEBUG("Failed to attach as only support uprobe");
            total_failed_probes += 2;
          }
        }
      }
    }
    DC_LOG_INFO(
        "Manual probes summary: total_manual_probes=%d, total_successful_probes=%d, "
        "total_failed_probes=%d",
        total_manual_probes, total_successful_probes, total_failed_probes);
  } else {
    DC_LOG_WARN("Failed to read probes file: %s", config_manager->manual_probe_path.c_str());
  }

#if defined(DATACRUMBS_ENABLE_INCLUSION_PATH) && (DATACRUMBS_ENABLE_INCLUSION_PATH == 1)
  int inclusion_trie = bpf_obj_get("/sys/fs/bpf/inclusion_path_trie");
  if (inclusion_trie < 0) {
    DC_LOG_ERROR("Failed to get inclusion path trie: %d", inclusion_trie);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  struct string_t* cur_key = NULL;
  struct string_t next_key = {};
  unsigned int value;
  for (;;) {
    err = bpf_map_get_next_key(inclusion_trie, cur_key, &next_key);
    if (err) break;
    bpf_map_delete_elem(inclusion_trie, &next_key);
    cur_key = &next_key;
  }

  // Get inclusion_path from configuration manager and build inclusion_list
  std::unordered_map<unsigned int, string_t> inclusion_list;
  inclusion_list.insert({1, {2, "/"}});  // Reserve index 1 for empty string
  std::string inclusion_paths = event_processor.configManager_->inclusion_path;
  if (!inclusion_paths.empty()) {
    std::stringstream ss(inclusion_paths);
    std::string path;
    unsigned int idx = 2;
    while (std::getline(ss, path, ':')) {
      if (!path.empty()) {
        string_t s;
        size_t copy_len = path.size();
        if (copy_len % 8 != 0) {
          copy_len = (copy_len / 8) * 8;
          if (copy_len == 0) copy_len = 2;
        }
        if (copy_len > sizeof(s.str) - 1) copy_len = sizeof(s.str) - 1;
        strncpy(s.str, path.c_str(), copy_len);
        s.str[copy_len - 1] = '\0';
        s.len = copy_len;
        inclusion_list[idx++] = s;
      }
    }
  }
  for (const auto& pair : inclusion_list) {
    if (bpf_map_update_elem(inclusion_trie, &pair.second, &pair.first, BPF_ANY) < 0) {
      DC_LOG_ERROR("Failed to update inclusion path trie for %s", pair.second.str);
      datacrumbs_bpf__destroy(skel);
      return 1;
    }
    DC_LOG_DEBUG("Added inclusion path: %s", path.c_str());
  }
  cur_key = NULL;
  next_key = {};
  for (;;) {
    err = bpf_map_get_next_key(inclusion_trie, cur_key, &next_key);
    if (err) break;

    bpf_map_lookup_elem(inclusion_trie, &next_key, &value);

    /* Use key and value here */
    DC_LOG_INFO("Trie key: %s, len: %u, value: %u", next_key.str, next_key.len, value);

    cur_key = &next_key;
  }
#endif
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  // Prepare context for event handler
  // Create ring buffer for event processing
  rb = ring_buffer__new(bpf_map__fd(skel->maps.output), handle_event, &event_processor, NULL);
  if (!rb) {
    err = -1;
    DC_LOG_ERROR("Failed to create ring buffer");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
#else
  INITIALIZE_MAP_1();
#ifdef GET_DATA_2_EXISTS
  INITIALIZE_MAP_2();
#endif
#ifdef GET_DATA_3_EXISTS
  INITIALIZE_MAP_3();
#endif
#ifdef GET_DATA_4_EXISTS
  int profile_4_fd = initialize_map_4(skel);
#endif
#ifdef GET_DATA_5_EXISTS
  int profile_5_fd = initialize_map_5(skel);
#endif
#ifdef GET_DATA_6_EXISTS
  int profile_6_fd = initialize_map_6(skel);
#endif
#ifdef GET_DATA_7_EXISTS
  int profile_7_fd = initialize_map_7(skel);
#endif
#ifdef GET_DATA_8_EXISTS
  int profile_8_fd = initialize_map_8(skel);
#endif
#ifdef GET_DATA_9_EXISTS
  int profile_9_fd = initialize_map_9(skel);
#endif
  int latest_interval_fd = bpf_map__fd(skel->maps.latest_interval);
  if (latest_interval_fd < 0) {
    DC_LOG_ERROR("Failed to get latest interval map fd: %d", latest_interval_fd);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
#endif
  int file_hash_fd = bpf_map__fd(skel->maps.file_map);
  if (file_hash_fd < 0) {
    DC_LOG_ERROR("Failed to get file hash fd: %d", file_hash_fd);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  double elapsed = timer.pauseTime();
  DC_LOG_PRINT("Initialization of DataCrumbs elapsed time: %f seconds", elapsed);
  DC_LOG_PRINT("Ready to run the code.");

  // Main event polling loop
  // Setup signal handler for Ctrl-C (SIGINT)
  static volatile bool stop = false;
  auto sig_handler = [](int) { stop = true; };
  signal(SIGINT, sig_handler);

  unsigned int batch_size = 1024;

  struct string_t* keys = (struct string_t*)malloc(batch_size * sizeof(struct string_t));
  unsigned int* values = (unsigned int*)malloc(batch_size * sizeof(unsigned int));
  // Initialize in_batch to NULL for the first iteration
  struct string_t* in_batch = NULL;

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 2)
  INITIALIZE_MAP_LOOKUP_1();
#ifdef GET_DATA_2_EXISTS
  INITIALIZE_MAP_LOOKUP_2();
#endif
#ifdef GET_DATA_3_EXISTS
  INITIALIZE_MAP_LOOKUP_3();
#endif
#ifdef GET_DATA_4_EXISTS
  INITIALIZE_MAP_LOOKUP_4();
#endif
#ifdef GET_DATA_5_EXISTS
  INITIALIZE_MAP_LOOKUP_5();
#endif
#ifdef GET_DATA_6_EXISTS
  INITIALIZE_MAP_LOOKUP_6();
#endif
#ifdef GET_DATA_7_EXISTS
  INITIALIZE_MAP_LOOKUP_7();
#endif
#ifdef GET_DATA_8_EXISTS
  INITIALIZE_MAP_LOOKUP_8();
#endif
#ifdef GET_DATA_9_EXISTS
  INITIALIZE_MAP_LOOKUP_9();
#endif
#endif

  unsigned long long last_processed_timestamp = 0;
  auto time_unit = 1000000000 / DATACRUMBS_TIME_INTERVAL_NS;
  while (!stop) {
    lookup_and_delete(file_hash_fd, &event_processor, keys, values, batch_size, in_batch);

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
    err = ring_buffer__poll(rb, 10);
    // Ctrl-C gives -EINTR
    if (err == -EINTR) {
      DC_LOG_INFO("\nReceived EINTR, exiting poll loop");
      err = 0;
      break;
    }
    if (err < 0) {
      DC_LOG_ERROR("Error polling ring buffer: %d", err);
      break;
    }
#else
    unsigned long long latest_ts = 0;
    err = bpf_map_lookup_elem(latest_interval_fd, &DATACRUMBS_TS_KEY, &latest_ts);
    if (err == 0) {
      if (last_processed_timestamp == 0) {
        last_processed_timestamp = latest_ts;
      }
      if (latest_ts - last_processed_timestamp > 0) {
        DC_LOG_DEBUG("Recieved latest latest_ts:%llu, last_processed_timestamp:%llu, interval:%d",
                     latest_ts, last_processed_timestamp, 0);
        last_processed_timestamp = latest_ts;

        LOOKUP_1_CALL();
#ifdef GET_DATA_2_EXISTS
        LOOKUP_2_CALL();
#endif
#ifdef GET_DATA_3_EXISTS
        LOOKUP_3_CALL();
#endif
#ifdef GET_DATA_4_EXISTS
        LOOKUP_4_CALL();
#endif
#ifdef GET_DATA_5_EXISTS
        LOOKUP_5_CALL();
#endif
#ifdef GET_DATA_6_EXISTS
        LOOKUP_6_CALL();
#endif
#ifdef GET_DATA_7_EXISTS
        LOOKUP_7_CALL();
#endif
#ifdef GET_DATA_8_EXISTS
        LOOKUP_8_CALL();
#endif
#ifdef GET_DATA_9_EXISTS
        LOOKUP_9_CALL();
#endif
      }
    }
#endif
    // Ctrl-C gives -EINTR
    if (err == -EINTR) {
      DC_LOG_INFO("\nReceived EINTR, exiting poll loop");
      err = 0;
      break;
    }
  }
  DC_LOG_INFO("\n");
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 2)
  DC_LOG_INFO("Collecting rest of the events");
  unsigned long long latest_ts = 0;
  DC_LOG_INFO("Collecting rest general events");
  while (LOOKUP_1_CALL() != -1);
#ifdef GET_DATA_2_EXISTS
  DC_LOG_INFO("Collecting rest sysio events");
  while (LOOKUP_2_CALL() != -1);
#endif
#ifdef GET_DATA_3_EXISTS
  DC_LOG_INFO("Collecting rest usdt events");
  while (LOOKUP_3_CALL() != -1);
#endif
#ifdef GET_DATA_4_EXISTS
  while (LOOKUP_4_CALL() != -1);
#endif
#ifdef GET_DATA_5_EXISTS
  while (LOOKUP_5_CALL() != -1);
#endif
#ifdef GET_DATA_6_EXISTS
  while (LOOKUP_6_CALL() != -1);
#endif
#ifdef GET_DATA_7_EXISTS
  while (LOOKUP_7_CALL() != -1);
#endif
#ifdef GET_DATA_8_EXISTS
  while (LOOKUP_8_CALL() != -1);
#endif
#ifdef GET_DATA_9_EXISTS
  while (LOOKUP_9_CALL() != -1);
#endif
#endif
  DC_LOG_PRINT("Collecting string metadata from file_map...");
  while (lookup_and_delete(file_hash_fd, &event_processor, keys, values, batch_size, in_batch) !=
         -1) {
    // Continue until no more keys are found
  }
  if (keys) {
    free(keys);
  }
  if (values) {
    free(values);
  }

  DC_LOG_PRINT("Finalizing DataCrumbs...");
  if (stop) {
    DC_LOG_INFO("Received SIGINT (Ctrl-C), exiting gracefully");
  }
  // Measure elapsed time for finalization and cleanup
  timer.resumeTime();

  // Finalize ChromeWriter instance
  event_processor.finalize();

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  // Cleanup resources
  ring_buffer__free(rb);
#endif
  datacrumbs_bpf__destroy(skel);

  double finalize_elapsed = timer.pauseTime();
  DC_LOG_PRINT("Finalization and cleanup of DataCrumbs elapsed time: %f seconds", finalize_elapsed);

  DC_LOG_TRACE("main: end");
  return -err;
}