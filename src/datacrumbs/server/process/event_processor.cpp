#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <datacrumbs/bpf/datacrumbs.skel.h>
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/logging.h>  // Logging header
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/typedefs.h>
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

#include <atomic>
#include <csignal>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

static datacrumbs::EventWithId* get_data_1(void* data, uint64_t index) {
  struct general_event_t* base = (general_event_t*)data;

  auto event = new datacrumbs::EventWithId(NORMAL_EVENT, index, base->type, base->id,
                                           base->event_id, base->ts, base->dur, nullptr);
  return event;
}

#define GET_DATA_3_EXISTS
static datacrumbs::EventWithId* get_data_3(void* data, uint64_t index) {
  struct usdt_event_t* base = (usdt_event_t*)data;
  auto args = new DataCrumbsArgs();
  args->emplace("clazz", base->class_hash);
  args->emplace("method", base->method_hash);
  auto event = new datacrumbs::EventWithId(NORMAL_EVENT, index, base->type, base->id,
                                           base->event_id, base->ts, base->dur, args);
  return event;
}

#define GET_DATA_FUNCTION(INDEX)                                       \
  auto write_event = get_data_##INDEX(data, event_index.fetch_add(1)); \
  if (write_event != nullptr) {                                        \
    writer->push_event(write_event);                                   \
  }

namespace datacrumbs {
class EventProcessor {
 public:
  EventProcessor(int argc, char** argv) {
    configManager_ =
        datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance(argc, argv);
    // Initialize the ChromeWriter singleton instance
    writer_ = datacrumbs::Singleton<datacrumbs::ChromeWriter>::get_instance();
    if (!writer_) {
      DC_LOG_ERROR("Failed to create ChromeWriter instance");
    }
  }
  int handle_event(void* data, size_t data_sz) {
    DC_LOG_TRACE("handle_event: start");

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
  int update_filename(const char* filename, unsigned int hash) {
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
  int finalize() {
    if (writer_) {
      writer_.reset();
    }
    return 0;
  }

 public:
  std::shared_ptr<ConfigurationManager> configManager_;
  std::shared_ptr<datacrumbs::ChromeWriter> writer_;

 private:
  std::atomic<uint64_t> event_index{0};  // Atomic index for event processing
};

}  // namespace datacrumbs

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
                                    unsigned int batch_size, struct string_t* in_batch,
                                    struct string_t* out_batch) {
  int ret =
      bpf_map_lookup_and_delete_batch(map_fd, in_batch, &in_batch, keys, values, &batch_size, 0);
  if (ret < 0 && errno != ENOENT) {
    perror("bpf_map_lookup_batch");
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
  // Prepare context for event handler
  // Create ring buffer for event processing
  rb = ring_buffer__new(bpf_map__fd(skel->maps.output), handle_event, &event_processor, NULL);
  if (!rb) {
    err = -1;
    DC_LOG_ERROR("Failed to create ring buffer");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  int file_hash_fd = bpf_map__fd(skel->maps.file_map);

  double elapsed = timer.pauseTime();
  DC_LOG_PRINT("Initialization of DataCrumbs elapsed time: %f seconds", elapsed);
  DC_LOG_PRINT("Ready to run the code.");

  // Main event polling loop
  // Setup signal handler for Ctrl-C (SIGINT)
  static volatile bool stop = false;
  auto sig_handler = [](int) { stop = true; };
  signal(SIGINT, sig_handler);

  unsigned int batch_size = 16;
  struct string_t* keys = (struct string_t*)malloc(batch_size * sizeof(struct string_t));
  unsigned int* values = (unsigned int*)malloc(batch_size * sizeof(unsigned int));
  // Initialize in_batch to NULL for the first iteration
  struct string_t* in_batch = NULL;
  struct string_t* out_batch = NULL;

  while (!stop) {
    lookup_and_delete(file_hash_fd, &event_processor, keys, values, batch_size, in_batch,
                      out_batch);
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
  }
  DC_LOG_PRINT("Collecting string metadata from file_map...");
  while (lookup_and_delete(file_hash_fd, &event_processor, keys, values, batch_size, in_batch,
                           out_batch) != -1) {
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

  // Cleanup resources
  ring_buffer__free(rb);
  datacrumbs_bpf__destroy(skel);

  double finalize_elapsed = timer.pauseTime();
  DC_LOG_PRINT("Finalization and cleanup of DataCrumbs elapsed time: %f seconds", finalize_elapsed);

  DC_LOG_TRACE("main: end");
  return -err;
}