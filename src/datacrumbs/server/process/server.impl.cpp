// Internal headers
#include <datacrumbs/common/enumerations.h>
#include <datacrumbs/server/process/event_processor.h>
// dependent headers
#include <mpi.h>
// std headers
#include <algorithm>
#include <string>

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
  int ret = bpf_map_lookup_and_delete_batch_compat(map_fd, in_batch, &in_batch, keys, values,
                                                   &batch_size, 0);
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
    return 0;
  }
  return 1;
}

inline static int lookup(int map_fd, datacrumbs::EventProcessor* event_processor,
                         struct string_t* keys, unsigned int* values, unsigned int batch_size,
                         struct string_t* in_batch) {
  int ret = bpf_map_lookup_batch_compat(map_fd, in_batch, &in_batch, keys, values, &batch_size, 0);
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

// Setup signal handler for Ctrl-C (SIGINT)
static volatile bool stop = false;
// Define a signal handler function with C linkage
static void sig_handler(int) {
  stop = true;
  DC_LOG_INFO("\nReceived SIGINT, setting loop variable");
}

static int manual_probes(datacrumbs::EventProcessor* event_processor, struct datacrumbs_bpf* skel) {
  auto config_manager = event_processor->configManager_;
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
  return 0;
}

static int sync_pipe[2];

/**
 * @brief Sends a signal to the parent process via a pipe to indicate completion.
 *
 * This function should be called by the child process after it has performed
 * its necessary startup and initialization tasks.
 */
static void daemon_notify_parent() {
  char signal_byte = '!';
  // The child process only needs the write end of the pipe.
  // The read end has already been closed.
  if (write(sync_pipe[1], &signal_byte, 1) == -1) {
    // If write fails, it's likely the parent has already exited, which is fine.
    if (errno != EPIPE) {
      perror("write error in daemon_notify_parent");
    }
  }
  // Always close the write end of the pipe after use.
  close(sync_pipe[1]);
}

/**
 * @brief Daemonizes the process, creating a new child and waiting for a signal.
 *
 * The initial parent process forks, and the child process continues the
 * daemonization sequence. The parent blocks until the child signals success,
 * then exits.
 *
 * @return Returns 0 in the daemon process, or a positive value representing
 *         the daemon's PID in the original parent process. Returns -1 on failure.
 */
static pid_t daemonize() {
  pid_t pid;

  // Create the pipe for synchronization before the first fork.
  if (pipe(sync_pipe) == -1) {
    perror("pipe error");
    return -1;
  }

  // First fork to detach from the controlling terminal.
  pid = fork();
  if (pid < 0) {
    perror("fork error");
    close(sync_pipe[0]);
    close(sync_pipe[1]);
    return -1;
  }

  // Parent process (the original caller).
  if (pid > 0) {
    // Parent closes the write end of the pipe.
    close(sync_pipe[1]);

    char signal_byte;
    // Wait to read a byte from the pipe. This blocks until the child writes.
    if (read(sync_pipe[0], &signal_byte, 1) == -1) {
      perror("read error in parent");
      close(sync_pipe[0]);
      return -1;
    }

    // Child signaled success. Parent can now exit cleanly.
    close(sync_pipe[0]);
    exit(EXIT_SUCCESS);
  }

  // First child process.
  // Close the read end of the pipe, as the child will only write.
  close(sync_pipe[0]);

  // Become a session leader to detach from the controlling terminal.
  if (setsid() < 0) {
    perror("setsid error");
    exit(EXIT_FAILURE);
  }

  // Second fork to ensure the daemon can't reacquire a controlling terminal.
  signal(SIGHUP, SIG_IGN);
  pid = fork();
  if (pid < 0) {
    perror("fork error");
    exit(EXIT_FAILURE);
  }

  // First child exits, orphaning the second child.
  if (pid > 0) {
    // The first child exits, leaving the second child as the daemon.
    exit(EXIT_SUCCESS);
  }

  // This code runs only in the second child (the actual daemon).

  // Close all open file descriptors.
  int x;
  for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
    if (x != sync_pipe[1]) {  // Keep the write end of the pipe open for the signal
      close(x);
    }
  }

  // Reopen standard file descriptors to /dev/null.
  open("/dev/null", O_RDWR);  // stdin
  dup(0);                     // stdout
  dup(0);                     // stderr

  return 0;  // Return 0 in the daemon process.
}

static std::string get_hostname() {
  char hostname[HOST_NAME_MAX];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    return std::string(hostname);
  }
  struct utsname uts;
  if (uname(&uts) == 0) {
    return std::string(uts.nodename);
  }
  return "unknownhost";
}

static std::string get_timestamp() {
  time_t now = time(nullptr);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&now));
  return std::string(buf);
}

static void redirect_output(const std::string& logfile, const std::string& user) {
  FILE* logf = freopen(logfile.c_str(), "a+", stdout);
  if (!logf) {
    perror("freopen failed to open log file");
    exit(EXIT_FAILURE);
  }
  freopen(logfile.c_str(), "a+", stderr);
  auto pwd = getpwnam(user.c_str());
  uid_t uid = pwd ? pwd->pw_uid : static_cast<uid_t>(-1);
  gid_t gid = pwd ? pwd->pw_gid : static_cast<gid_t>(-1);
  // Set file ownership to user
  chown(logfile.c_str(), uid, gid);
  // Optionally set permissions (e.g., rw-r-----)
  chmod(logfile.c_str(), 0640);
}

static void write_pid_file(const std::string& pidfile, const std::string& user) {
  if (access(pidfile.c_str(), F_OK) == 0) {
    remove(pidfile.c_str());
  }
  std::ofstream ofs(pidfile);
  ofs << getpid() << std::endl;
  ofs.close();
  auto pwd = getpwnam(user.c_str());
  uid_t uid = pwd ? pwd->pw_uid : static_cast<uid_t>(-1);
  gid_t gid = pwd ? pwd->pw_gid : static_cast<gid_t>(-1);
  // Set file ownership to user
  chown(pidfile.c_str(), uid, gid);
  // Optionally set permissions (e.g., rw-r-----)
  chmod(pidfile.c_str(), 0640);
}

static int main_process(int argc, char** argv, datacrumbs::EventProcessor* event_processor,
                        bool notify_parent = false) {
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

  // Attach BPF skeleton
  err = datacrumbs_bpf__attach(skel);
  if (err) {
    DC_LOG_ERROR("Failed to attach BPF skeleton: %d", err);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  manual_probes(event_processor, skel);
#if !(defined(DATACRUMBS_ENABLE) && (DATACRUMBS_ENABLE == 1))
  DC_LOG_WARN("DATACRUMBS_ENABLE_OPT is set to OFF. Nothing will be captured");
#endif
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  DC_LOG_PRINT("DataCrumbs in Tracer mode");
#else
  DC_LOG_PRINT("DataCrumbs in Profiler mode");
#endif

#if defined(DATACRUMBS_ENABLE_INCLUSION_PATH) && (DATACRUMBS_ENABLE_INCLUSION_PATH == 1)
  int inclusion_trie = bpf_map__fd(skel->maps.inclusion_path_trie);
  if (inclusion_trie < 0) {
    DC_LOG_ERROR("Failed to get inclusion path trie: %d", inclusion_trie);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  struct string_t* cur_key = NULL;
  struct string_t next_key = {};
  struct string_t value;
  for (;;) {
    err = bpf_map_get_next_key(inclusion_trie, cur_key, &next_key);
    if (err) break;
    bpf_map_delete_elem(inclusion_trie, &next_key);
    cur_key = &next_key;
  }

  // Get inclusion_path from configuration manager and build inclusion_list
  std::unordered_map<unsigned int, string_t> inclusion_list;
  std::string inclusion_paths = event_processor->configManager_->inclusion_paths;
  if (!inclusion_paths.empty()) {
    std::stringstream ss(inclusion_paths);
    std::string path;
    unsigned int idx = 1;
    while (std::getline(ss, path, ':')) {
      if (!path.empty()) {
        string_t s;
        size_t copy_len = path.size();
        if (copy_len > sizeof(s.str) - 1) copy_len = sizeof(s.str) - 1;
        strncpy(s.str, path.c_str(), copy_len);
        s.str[copy_len] = '\0';
        s.len = copy_len * 8;
        inclusion_list[idx++] = s;
      }
    }
  }
  for (const auto& pair : inclusion_list) {
    if (bpf_map_update_elem(inclusion_trie, &pair.second, &pair.second, BPF_ANY) < 0) {
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
  int failed_events_fd = bpf_map__fd(skel->maps.failed_request);
  if (failed_events_fd < 0) {
    DC_LOG_ERROR("Failed to get failed events fd: %d", failed_events_fd);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  // Prepare context for event handler
  // Create ring buffer for event processing
  rb = ring_buffer__new(bpf_map__fd(skel->maps.output), handle_event, event_processor, NULL);
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

  if (event_processor->configManager_->mpi_rank == 0) {
    DC_LOG_PRINT("Initialization of DataCrumbs elapsed time: %f seconds", elapsed);
  }

  if (notify_parent) daemon_notify_parent();
  // Main event polling loop
  signal(SIGINT, sig_handler);
  if (event_processor->configManager_->mpi_rank == 0) {
    std::string ready_file = "/tmp/datacrumbs_" + event_processor->configManager_->user + "_" +
                             event_processor->configManager_->run_id + ".ready";
    std::ofstream ofs_ready(ready_file);
    ofs_ready << "ready" << std::endl;
    ofs_ready.close();
    auto pwd = getpwnam(event_processor->configManager_->user.c_str());
    uid_t uid = pwd ? pwd->pw_uid : static_cast<uid_t>(-1);
    gid_t gid = pwd ? pwd->pw_gid : static_cast<gid_t>(-1);
    chown(ready_file.c_str(), uid, gid);
    chmod(ready_file.c_str(), 0660);
    DC_LOG_PRINT("Server running on %d nodes. Ready to run the code.",
                 event_processor->configManager_->mpi_size);
  }
  MPI_Barrier(MPI_COMM_WORLD);

  unsigned int batch_size = 1024;
#if defined(DATACRUMBS_BPFTIME_COMPATIBLE_FLAG) && (DATACRUMBS_BPFTIME_COMPATIBLE_FLAG == 0)

  struct string_t* keys = (struct string_t*)malloc(batch_size * sizeof(struct string_t));
  unsigned int* values = (unsigned int*)malloc(batch_size * sizeof(unsigned int));
  // Initialize in_batch to NULL for the first iteration
  struct string_t* in_batch = NULL;
#endif

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
    err = 0;
#if defined(DATACRUMBS_BPFTIME_COMPATIBLE_FLAG) && (DATACRUMBS_BPFTIME_COMPATIBLE_FLAG == 0)
    err = lookup_and_delete(file_hash_fd, event_processor, keys, values, batch_size, in_batch);
    if (err == -EINTR) {
      DC_LOG_INFO("\nReceived EINTR, exiting poll loop");
      err = 0;
      break;
    }
#endif
    err = 0;
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
    int failed_events = 0;
    err = bpf_map_lookup_elem(failed_events_fd, &DATACRUMBS_FAILED_EVENTS_KEY, &failed_events);
    if (err == 0) {
      event_processor->failed_events = failed_events;
    }
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
  batch_size = 1024 * 1024;
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
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  int failed_events = 0;
  err = bpf_map_lookup_elem(failed_events_fd, &DATACRUMBS_FAILED_EVENTS_KEY, &failed_events);
  if (err == 0 && event_processor->configManager_->mpi_rank == 0) {
    DC_LOG_PRINT("Total %d events failed", failed_events);
  }
#endif

#if defined(DATACRUMBS_BPFTIME_COMPATIBLE_FLAG) && (DATACRUMBS_BPFTIME_COMPATIBLE_FLAG == 0)
  if (event_processor->configManager_->mpi_rank == 0) {
    DC_LOG_PRINT("Collecting string metadata from file_map...");
  }
  while (lookup_and_delete(file_hash_fd, event_processor, keys, values, batch_size, in_batch) ==
         1) {
    // Continue until no more keys are found
  }
  if (keys) {
    free(keys);
  }
  if (values) {
    free(values);
  }
#endif
  if (event_processor->configManager_->mpi_rank == 0) {
    DC_LOG_PRINT("Finalizing DataCrumbs...");
  }
  if (stop) {
    DC_LOG_INFO("Received SIGINT (Ctrl-C), exiting gracefully");
  }
  // Measure elapsed time for finalization and cleanup
  timer.resumeTime();

  // Finalize ChromeWriter instance
  event_processor->finalize();

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
  // Cleanup resources
  ring_buffer__free(rb);
#endif
  datacrumbs_bpf__destroy(skel);

  double finalize_elapsed = timer.pauseTime();
  double sum_elapsed = 0.0, min_elapsed = 0.0, max_elapsed = 0.0;
  MPI_Reduce(&finalize_elapsed, &sum_elapsed, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&finalize_elapsed, &min_elapsed, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&finalize_elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  if (event_processor->configManager_->mpi_rank == 0) {
    DC_LOG_PRINT("Finalization and cleanup of DataCrumbs: average=%f, min=%f, max=%f over %d ranks",
                 sum_elapsed / event_processor->configManager_->mpi_size, min_elapsed, max_elapsed,
                 event_processor->configManager_->mpi_size);
  }

  // Gather failed_events and total_events across all ranks
  int local_failed_events = event_processor->failed_events;
  int global_failed_events_sum = 0, global_failed_events_min = 0, global_failed_events_max = 0;
  MPI_Reduce(&local_failed_events, &global_failed_events_sum, 1, MPI_INT, MPI_SUM, 0,
             MPI_COMM_WORLD);
  MPI_Reduce(&local_failed_events, &global_failed_events_min, 1, MPI_INT, MPI_MIN, 0,
             MPI_COMM_WORLD);
  MPI_Reduce(&local_failed_events, &global_failed_events_max, 1, MPI_INT, MPI_MAX, 0,
             MPI_COMM_WORLD);

  int local_total_events = static_cast<int>(event_processor->event_index.load());
  int global_total_events_sum = 0, global_total_events_min = 0, global_total_events_max = 0;
  MPI_Reduce(&local_total_events, &global_total_events_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_total_events, &global_total_events_min, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_total_events, &global_total_events_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  if (event_processor->configManager_->mpi_rank == 0) {
    DC_LOG_PRINT("Failed events: sum=%d, min=%d, max=%d", global_failed_events_sum,
                 global_failed_events_min, global_failed_events_max);
    DC_LOG_PRINT("Total events: sum=%d, min=%d, max=%d", global_total_events_sum,
                 global_total_events_min, global_total_events_max);
    // Write status info to JSON file for postprocessing
    std::string status_file = "/tmp/datacrumbs_" + event_processor->configManager_->user +
                              "_status_" + event_processor->configManager_->run_id + ".json";
    struct json_object* status_json = json_object_new_object();
    json_object_object_add(
        status_json, "trace_file",
        json_object_new_string(event_processor->configManager_->trace_file_path.c_str()));
    json_object_object_add(status_json, "total_events_captured",
                           json_object_new_int64(event_processor->event_index.load()));
    json_object_object_add(status_json, "events_failed",
                           json_object_new_int(event_processor->failed_events));

    if (json_object_to_file_ext(status_file.c_str(), status_json, JSON_C_TO_STRING_PRETTY) != 0) {
      DC_LOG_ERROR("Failed to write status file: %s", status_file.c_str());
    }
    json_object_put(status_json);
    auto pwd = getpwnam(event_processor->configManager_->user.c_str());
    auto uid = pwd ? pwd->pw_uid : static_cast<uid_t>(-1);
    auto gid = pwd ? pwd->pw_gid : static_cast<gid_t>(-1);
    chown(status_file.c_str(), uid, gid);
    chmod(status_file.c_str(), 0660);
  }

  DC_LOG_TRACE("main: end");
  return 0;
}

static int main_call(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  auto event_processor = datacrumbs::EventProcessor(argc, argv);
  std::string hostname = get_hostname();
  std::string pidfile =
      "/tmp/datacrumbs_" + hostname + "_" + event_processor.configManager_->run_id + ".pid";

  int return_code = 0;
  if (event_processor.configManager_->mpi_rank == 0 &&
      event_processor.configManager_->exe_mode != datacrumbs::ExecutableMode::STOP) {
    event_processor.configManager_->print_configurations();
  }
  if (event_processor.configManager_->exe_mode == datacrumbs::ExecutableMode::RUN) {
    write_pid_file(pidfile, event_processor.configManager_->user);
    return_code = main_process(argc, argv, &event_processor, false);
  } else if (event_processor.configManager_->exe_mode == datacrumbs::ExecutableMode::START) {
    daemonize();
    std::string logfile = event_processor.configManager_->log_dir + "/datacrumbs_" + hostname +
                          "_" + event_processor.configManager_->run_id + ".log";
    redirect_output(logfile, event_processor.configManager_->user);
    DC_LOG_PRINT("Spawned daemon with pid %d, output redirected to %s\n", getpid(),
                 logfile.c_str());
    write_pid_file(pidfile, event_processor.configManager_->user);
    return_code = main_process(argc, argv, &event_processor, true);
  } else if (event_processor.configManager_->exe_mode == datacrumbs::ExecutableMode::STOP) {
    // Find and kill daemon by pid file
    std::ifstream ifs(pidfile);
    pid_t pid = 0;
    ifs >> pid;
    ifs.close();
    if (pid > 0) {
      kill(pid, SIGINT);
      int status = 0;
      if (pid > 0) {
        // Wait for the process to terminate after sending SIGINT
        // Check if process exists before calling waitpid
        // Poll for process termination using ps
        int max_retries = 600;  // Wait up to ~600 seconds (1s * 600)
        const char* user = getenv("USER");
        if (!user) {
          user = "unknown";
        }
        if (event_processor.configManager_->mpi_rank == 0) {
          DC_LOG_PRINT("Sent SIGINT as %s. Waiting for %f minutes for pid:%d to exit", user,
                       max_retries / 60.0, pid);
        }
        int i;
        for (i = 0; i < max_retries; ++i) {
          std::string ps_cmd = "ps -p " + std::to_string(pid) + " > /dev/null";
          int ret = system(ps_cmd.c_str());
          if (ret != 0) {
            // Process no longer exists
            break;
          }
          usleep(1000000);  // Sleep 1s
        }
        if (i == max_retries) {
          DC_LOG_PRINT("Process %d did not terminate within the expected time for rank %d.", pid,
                       event_processor.configManager_->mpi_rank);
          return_code = 1;
        } else {
          if (event_processor.configManager_->mpi_rank == 0) {
            DC_LOG_PRINT("Process %d has terminated.", pid);
          }
        }
      }
      MPI_Barrier(MPI_COMM_WORLD);
      if (access(pidfile.c_str(), F_OK) == 0) {
        remove(pidfile.c_str());
      }
    } else {
      DC_LOG_ERROR("Could not find pid to stop on rank %d.\n",
                   event_processor.configManager_->mpi_rank);
    }
  }
  MPI_Finalize();
  return (return_code);
}