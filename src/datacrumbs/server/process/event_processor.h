#ifndef DATACRUMBS_SERVER_PROCESS_DEF
#define DATACRUMBS_SERVER_PROCESS_DEF
// BPF Headers
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
// Generated Headers
#include <datacrumbs/bpf/datacrumbs.skel.h>
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/constants.h>
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/logging.h>  // Logging header
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/typedefs.h>
#include <datacrumbs/common/utils.h>
#include <datacrumbs/server/bpf/shared.h>
#include <datacrumbs/server/process/writer/chrome_writer.h>
// std headers
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
#include <unordered_set>
#include <utility>

namespace datacrumbs {
class EventProcessor {
 public:
  EventProcessor(int argc, char** argv);

  ~EventProcessor() {}

  int handle_event(void* data, size_t data_sz);

  int update_filename(const char* filename, unsigned int hash);

  int capture_general_counter(struct profile_key_t* key, struct profile_value_t* value) {
    return 0;
  }

  int capture_usdt_counter(struct usdt_profile_key_t* key, struct profile_value_t* value) {
    return 0;
  }

  int finalize() {
    if (writer_) {
      writer_->finalize();
    }
    return 0;
  }

 public:
  std::shared_ptr<ConfigurationManager> configManager_;
  std::shared_ptr<datacrumbs::ChromeWriter> writer_;
  int failed_events;  // Count of failed events

 private:
  std::atomic<uint64_t> event_index{0};                // Atomic index for event processing
  std::unordered_set<unsigned int> processed_hashes_;  // Set to track processed PIDs
};

}  // namespace datacrumbs

#endif