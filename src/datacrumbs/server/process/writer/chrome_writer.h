#pragma once

#include <datacrumbs/common/configuration_manager.h>
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/common/typdefs.h>
#include <datacrumbs/server/bpf/shared.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <any>
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

struct EventWithId {
  struct general_event_t* event;
  unsigned long long event_id;
  DataCrumbsArgs* args;
  EventWithId(struct general_event_t* e, uint64_t id,
              DataCrumbsArgs* a)
      : event(e), event_id(id), args(a) {}
};
namespace datacrumbs {

class ChromeWriter {
 public:
  // Create a ChromeWriter that writes to the given filename.
  ChromeWriter() : stop_flag_(false) {
    auto configManager_ = datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance();
    file_ = std::fopen(configManager_->trace_file_path.c_str(), "a+");

    if (file_) {
      auto pwd = getpwnam(configManager_->user.c_str());
      uid_t uid = pwd ? pwd->pw_uid : static_cast<uid_t>(-1);
      gid_t gid = pwd ? pwd->pw_gid : static_cast<gid_t>(-1);
      // Set file ownership to configManager_->user
      chown(configManager_->trace_file_path.c_str(), uid, gid);
      // Optionally set permissions (e.g., rw-r-----)
      chmod(configManager_->trace_file_path.c_str(), 0640);
    }
    if (file_) {
      buffer_ = new char[32 * 1024 * 1024];
      std::setvbuf(file_, buffer_, _IOLBF, 32 * 1024 * 1024);
      std::fprintf(file_, "[\n");
      first_event_ = true;
    }
    worker_ = std::thread([this]() { this->worker_loop(); });
  }

  // Destructor flushes and closes the file, and joins the worker thread.
  ~ChromeWriter() {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      stop_flag_ = true;
    }
    queue_cv_.notify_one();
    if (worker_.joinable()) worker_.join();

    if (file_) {
      std::fprintf(file_, "\n]\n");
      std::fclose(file_);
    }
    delete[] buffer_;
  }

  void push_event(EventWithId event) {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      event_queue_.emplace_back(event);
    }
    queue_cv_.notify_one();
  }

 private:
  // Serialize and write a single event to the file, including event_id as "id".
  void write_event(const EventWithId& event_with_id) {
    if (!file_) return;
    auto configManager_ = datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance();
    auto event = event_with_id.event;
    uint64_t event_id = event_with_id.event_id;
    auto args = event_with_id.args;

    unsigned int pid = event->id;
    auto it = configManager_->category_map.find(event->event_id);
    if (it != configManager_->category_map.end()) {
      const auto& [probe_name, function_name] = it->second;
      char buffer[1024];
      int len = std::snprintf(
          buffer, sizeof(buffer),
          R"({"id":%llu,"name":"%s","cat":"%s","ph":"X","ts":%llu,"dur":%llu,"pid":%d,"tid":%d)",
          static_cast<unsigned long long>(event_id), function_name.c_str(), probe_name.c_str(),
          static_cast<unsigned long long>(event->ts),
          static_cast<unsigned long long>(event->dur / 1000), pid, pid);

      std::string args_json = "{";
      bool first = true;
      for (const auto& [key, value] : *args) {
        if (!first) args_json += ",";
        args_json += "\"";
        args_json += key;
        args_json += "\":";
        if (value.type() == typeid(int)) {
          args_json += std::to_string(std::any_cast<int>(value));
        } else if (value.type() == typeid(unsigned int)) {
          args_json += std::to_string(std::any_cast<unsigned int>(value));
        } else if (value.type() == typeid(uint64_t)) {
          args_json += std::to_string(std::any_cast<uint64_t>(value));
        } else if (value.type() == typeid(float)) {
          args_json += std::to_string(std::any_cast<float>(value));
        } else if (value.type() == typeid(double)) {
          args_json += std::to_string(std::any_cast<double>(value));
        } else if (value.type() == typeid(const char*)) {
          args_json += "\"";
          args_json += std::any_cast<const char*>(value);
          args_json += "\"";
        } else if (value.type() == typeid(std::string)) {
          args_json += "\"";
          args_json += std::any_cast<std::string>(value);
          args_json += "\"";
        } else {
          args_json += "\"<unsupported>\"";
        }
        first = false;
      }
      args_json += "}";

      {
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (!first_event_) {
          std::fprintf(file_, ",\n");
        } else {
          first_event_ = false;
        }
        std::string event_json = std::string(buffer, len) + ",\"args\":" + args_json + "}";
        std::fwrite(event_json.c_str(), 1, event_json.size(), file_);
        std::fflush(file_);
      }
    }
  }
  void worker_loop() {
    while (true) {
      EventWithId event_with_id(nullptr, 0, nullptr);
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return !event_queue_.empty() || stop_flag_; });
        if (event_queue_.empty() && stop_flag_) {
          break;
        }
        if (!event_queue_.empty()) {
          event_with_id = event_queue_.front();
          event_queue_.pop_front();
        } else {
          continue;
        }
      }
      if (event_with_id.event != nullptr) {
        write_event(event_with_id);
      }
    }
  }

  FILE* file_ = nullptr;
  char* buffer_ = nullptr;
  bool first_event_ = true;

  std::mutex file_mutex_;

  std::deque<EventWithId> event_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::thread worker_;
  bool stop_flag_;
};

}  // namespace datacrumbs