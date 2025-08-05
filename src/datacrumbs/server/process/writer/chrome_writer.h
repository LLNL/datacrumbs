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
      buffer_ = new char[4 * 1024];
      std::setvbuf(file_, buffer_, _IOLBF, 4 * 1024);
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
      std::fprintf(file_, "]");
      std::fclose(file_);
    }
    delete[] buffer_;
  }

  void push_event(EventWithId* event) {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      event_queue_.emplace_back(event);
    }
    queue_cv_.notify_one();
  }

  // Serialize and write a single event to the file, including event_id as "id".
  void write_event(EventWithId* event_with_id) {
    if (!file_) return;
    auto configManager_ = datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance();
    uint64_t index = event_with_id->index;
    auto args = event_with_id->args;

    unsigned int pid = event_with_id->tgid_pid;
    unsigned int tid = event_with_id->tgid_pid >> 32;
    auto it = configManager_->category_map.find(event_with_id->event_id);
    if (it != configManager_->category_map.end()) {
      std::string probe_name;
      std::string function_name;
      if (event_with_id->type == 3) {
        probe_name = "usdt";

        if (args != nullptr) {
          std::string clazz, method;
          method = std::any_cast<std::string>((*args)["method"]);
          clazz = std::any_cast<std::string>((*args)["clazz"]);
          if (clazz.size() > 0) {
            auto dot_py = clazz.rfind(".py");
            if (dot_py != std::string::npos) {
              // Ends with .py, remove extension
              clazz = clazz.substr(0, dot_py);
            }
            // Not ending with .py, extract after last . or /
            auto last_dot = clazz.find_last_of('.');
            auto last_slash = clazz.find_last_of("/\\");
            size_t pos = std::string::npos;
            if (last_dot != std::string::npos && last_slash != std::string::npos) {
              pos = std::max(last_dot, last_slash);
            } else if (last_dot != std::string::npos) {
              pos = last_dot;
            } else if (last_slash != std::string::npos) {
              pos = last_slash;
            }
            if (pos != std::string::npos && pos + 1 < clazz.size()) {
              clazz = clazz.substr(pos + 1);
            }
          }
          function_name = clazz + "." + method;
          args->erase("clazz");
          args->erase("method");
        } else {
          function_name = "unknown";
        }
      } else {
        probe_name = it->second.first;
        function_name = it->second.second;
      }
      char buffer[1024];
      unsigned long long ts_us = 0;
      if (event_with_id->ts > std::numeric_limits<unsigned long long>::max() / 1000) {
        ts_us = std::numeric_limits<unsigned long long>::max();
      } else {
        ts_us = static_cast<unsigned long long>(std::floor(event_with_id->ts / 1000.0));
      }
      unsigned long long dur_us = 0;
      if (event_with_id->dur > std::numeric_limits<unsigned long long>::max() / 1000) {
        dur_us = std::numeric_limits<unsigned long long>::max();
      } else {
        dur_us = static_cast<unsigned long long>(std::ceil(event_with_id->dur / 1000.0));
      }
      int len = std::snprintf(
          buffer, sizeof(buffer),
          R"({"id":%lu,"name":"%s","cat":"%s","ph":"X","ts":%llu,"dur":%llu,"pid":%d,"tid":%d)",
          index, function_name.c_str(), probe_name.c_str(), ts_us, dur_us, pid, tid);

      std::string args_json = "{";

      bool first = true;
      if (args != nullptr && !args->empty()) {
        for (auto pair : *args) {
          const std::string& key = pair.first;
          const std::any& value = pair.second;
          if (!first) args_json += ",";
          args_json += "\"";
          args_json += key;
          args_json += "\":";
          if (value.type() == typeid(int)) {
            args_json += std::to_string(std::any_cast<int>(value));
          } else if (value.type() == typeid(unsigned long long)) {
            args_json += std::to_string(std::any_cast<unsigned long long>(value));
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
      }
      args_json += "}";

      {
        std::lock_guard<std::mutex> lock(file_mutex_);
        std::string event_json;
        if (first) {
          event_json = std::string(buffer, len) + "}\n";
        } else {
          event_json = std::string(buffer, len) + ",\"args\":" + args_json + "}\n";
        }
        DC_LOG_DEBUG("Writing event: %s", event_json.c_str());
        std::fwrite(event_json.c_str(), 1, event_json.size(), file_);
        std::fflush(file_);
      }
    }
    if (args != nullptr) {
      delete args;  // Clean up args after use
    }
    if (event_with_id != nullptr) {
      delete event_with_id;  // Clean up event after writing
    }
  }

 private:
  void worker_loop() {
    while (true) {
      EventWithId* event_with_id = nullptr;
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
      if (event_with_id != nullptr) {
        write_event(event_with_id);
      }
    }
  }

  FILE* file_ = nullptr;
  char* buffer_ = nullptr;
  bool first_event_ = true;

  std::mutex file_mutex_;

  std::deque<EventWithId*> event_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::thread worker_;
  bool stop_flag_;
};

}  // namespace datacrumbs