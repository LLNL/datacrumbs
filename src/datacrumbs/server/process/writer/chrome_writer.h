#pragma once
// Generated Headers
#include <datacrumbs/datacrumbs_config.h>
// Other headers
#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/constants.h>
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/typedefs.h>
#include <datacrumbs/server/bpf/shared.h>
#include <datacrumbs/server/process/compress/zlib_compressor.h>
// std headers
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <any>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

/*
Example usage:

#include "chrome_writer.h"

int main() {
        ChromeWriter writer("trace.json");

        generic_event_t evt = {"my_event", "category", 'B', 123456, 1, 2};
        writer.push_event(evt, "foo", 42, "bar", 3.14, "desc", "hello");

        // ... do more work ...

        // Writer will flush and close on destruction
        return 0;
}
*/

namespace datacrumbs {

class ChromeWriter {
 public:
  // Create a ChromeWriter that writes to the given filename.
  ChromeWriter();

  // Destructor flushes and closes the file, and joins the worker thread.
  ~ChromeWriter();

  void push_event(EventWithId* event);

  // Serialize and write a single event to the file, including event_id as "id".
  void write_event(EventWithId* event_with_id);

  void finalize();

 private:
  void worker_loop();

  bool first_event_ = true;

  std::mutex file_mutex_;

  std::deque<EventWithId*> event_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::thread worker_;
  bool stop_flag_;
  unsigned long index_;
  ZlibCompression* compressor_;
  size_t chunk_size_;
};

}  // namespace datacrumbs