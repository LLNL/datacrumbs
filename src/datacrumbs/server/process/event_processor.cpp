#include <bpf/libbpf.h>
#include <datacrumbs/bpf/datacrumbs.skel.h>
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/logging.h>  // Logging header
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/utils.h>
#include <datacrumbs/server/bpf/shared.h>
#include <datacrumbs/server/process/generated_process.h>
#include <datacrumbs/server/process/writer/chrome_writer.h>
// #include <datacrumbs/server/process/
#include <errno.h>
#include <grp.h>
#include <json-c/json.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <csignal>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

static DataCrumbsArgs empty_args{};

// Custom libbpf print function for debugging
static int libbpf_print_fn(enum libbpf_print_level level, const char* format, va_list args) {
  if (level >= LIBBPF_DEBUG) return 0;
  return vfprintf(stderr, format, args);
}

// Event handler for BPF ring buffer events
int handle_event(void* ctx, void* data, size_t data_sz) {
  DC_LOG_TRACE("handle_event: start");
  auto configManager_ = datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance();

  // Static variables for trace file handling
  static uint64_t event_index = 0;

  struct general_event_t* event = (general_event_t*)data;
  unsigned int pid = event->id;

  if (pid == 0) {
    DC_LOG_DEBUG("handle_event: pid is 0, skipping event");
    return 0;
  }
  auto it = configManager_->category_map.find(event->event_id);
  if (it != configManager_->category_map.end()) {
    const auto& [probe_name, function_name] = it->second;
    // Print event info to stdout for debugging
    DC_LOG_DEBUG("%-6u  %-6llu %-6llu %-6llu  %s.%s", pid, event->event_id, event->ts, event->dur,
                 probe_name.c_str(), function_name.c_str());
    // Write event to Chrome trace file
    auto writer = datacrumbs::Singleton<datacrumbs::ChromeWriter>::get_instance();
    if (!writer) {
      DC_LOG_ERROR("Failed to create ChromeWriter instance");
      return 1;
    }
    if (event->type > 0) {
      if (event->type == 1) {
        writer->push_event(EventWithId(event, event_index++, &empty_args));  // Additional args
      } else if (event->type == 2) {
#ifdef GET_DATA_2_EXISTS
        // For event type 2, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_2(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 3) {
#ifdef GET_DATA_3_EXISTS
        // For event type 3, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_3(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 4) {
#ifdef GET_DATA_4_EXISTS
        // For event type 4, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_4(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 5) {
#ifdef GET_DATA_5_EXISTS
        // For event type 5, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_5(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 6) {
#ifdef GET_DATA_6_EXISTS
        // For event type 6, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_6(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 7) {
#ifdef GET_DATA_7_EXISTS
        // For event type 7, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_7(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 8) {
#ifdef GET_DATA_8_EXISTS
        // For event type 8, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_8(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 9) {
#ifdef GET_DATA_9_EXISTS
        // For event type 9, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_9(event);
        args.emplace_back("event_type", event->type);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else if (event->type == 10) {
#ifdef GET_DATA_10_EXISTS
        // For event type 10, we can pass additional arguments if needed
        DataCrumbsArgs args = get_data_10(event);
        writer->push_event(EventWithId(event, event_index++, &args));
#else
        DC_LOG_WARN("Unknown event type: %u, skipping event", event->type);
#endif
      } else {
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

// Handler for lost events in the ring buffer
void lost_event(void* ctx, int cpu, long long unsigned int data_sz) {
  DC_LOG_WARN("lost event on cpu %d, size %llu", cpu, data_sz);
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
  auto configManager_ =
      datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance(argc, argv);
  if (!configManager_) {
    DC_LOG_ERROR("Failed to get configuration manager instance");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  if (configManager_->category_map.empty()) {
    DC_LOG_ERROR("Category map is empty, cannot proceed");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  auto writer = datacrumbs::Singleton<datacrumbs::ChromeWriter>::get_instance();
  if (!writer) {
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

  // Prepare context for event handler

  // Create ring buffer for event processing
  rb = ring_buffer__new(bpf_map__fd(skel->maps.output), handle_event, NULL, NULL);
  if (!rb) {
    err = -1;
    DC_LOG_ERROR("Failed to create ring buffer");
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

  while (!stop) {
    err = ring_buffer__poll(rb, 30000);
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
  }
  if (stop) {
    DC_LOG_INFO("Received SIGINT (Ctrl-C), exiting gracefully");
  }
  // Measure elapsed time for finalization and cleanup
  timer.resumeTime();

  // Finalize ChromeWriter instance
  if (writer) {
    writer.reset();
  }
  // Cleanup resources
  ring_buffer__free(rb);
  datacrumbs_bpf__destroy(skel);

  double finalize_elapsed = timer.pauseTime();
  DC_LOG_PRINT("Finalization and cleanup of DataCrumbs elapsed time: %f seconds", finalize_elapsed);

  DC_LOG_TRACE("main: end");
  return -err;
}