#ifndef DATACRUMBS_SERVER_PROCESS_DEF
#define DATACRUMBS_SERVER_PROCESS_DEF
// BPF Headers
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
// Generated Headers
#include <datacrumbs/common/logging.h>
#include <datacrumbs/datacrumbs_config.h>
// Internal Headers
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/runtime_configuration_manager.h>
#include <datacrumbs/common/utils.h>
#include <datacrumbs/server/bpf/compat/map.h>
#include <datacrumbs/server/bpf/shared.h>
#include <datacrumbs/server/process/writer/chrome_writer.h>

// std headers
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <json-c/json.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace datacrumbs {
class EventProcessor {
 public:
  explicit EventProcessor(const std::filesystem::path& probe_file);

  ~EventProcessor() {}

  int handle_event(void* data, size_t data_sz);

  int update_filename(const char* filename, unsigned int hash);

  int capture_general_counter(struct profile_key_t* key, struct profile_value_t* value) {
    return 0;
  }

  int capture_usdt_counter(struct usdt_profile_key_t* key, struct profile_value_t* value) {
    return 0;
  }

  int finalize();

 public:
  std::shared_ptr<RuntimeConfigurationManager> configManager_;
  std::shared_ptr<datacrumbs::ChromeWriter> writer_;
  int failed_events;  // Count of failed events
  std::atomic<uint64_t> event_index{0};

 private:                                              // Atomic index for event processing
  std::unordered_set<unsigned int> processed_hashes_;  // Set to track processed PIDs
};

}  // namespace datacrumbs

#endif
