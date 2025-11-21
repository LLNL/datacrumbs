

#include <datacrumbs/server/process/writer/chrome_writer.h>

// Specialization of the Singleton instance for KSymCapture.
// This holds the shared pointer to the singleton instance.
template <>
std::shared_ptr<datacrumbs::ChromeWriter>
    datacrumbs::Singleton<datacrumbs::ChromeWriter>::instance = nullptr;

// Specialization of the flag to stop creating new instances of KSymCapture.
template <>
bool datacrumbs::Singleton<datacrumbs::ChromeWriter>::stop_creating_instances = false;

namespace datacrumbs {
ChromeWriter::ChromeWriter() : stop_flag_(false), chunk_size_(16 * 1024 * 1024) {
  auto configManager_ = datacrumbs::Singleton<datacrumbs::ConfigurationManager>::get_instance();
  compressor_ = new ZlibCompression(configManager_->trace_file_path, chunk_size_);
  // file_ = std::fopen(configManager_->trace_file_path.c_str(), "a+");
  auto pwd = getpwnam(configManager_->user.c_str());
  uid_t uid = pwd ? pwd->pw_uid : static_cast<uid_t>(-1);
  gid_t gid = pwd ? pwd->pw_gid : static_cast<gid_t>(-1);
  // Set file ownership to configManager_->user
  chown(configManager_->trace_file_path.c_str(), uid, gid);
  // Optionally set permissions (e.g., rw-r-----)
  chmod(configManager_->trace_file_path.c_str(), 0660);
  compressor_->compress("[\n");
  first_event_ = true;
  worker_ = std::thread([this]() { this->worker_loop(); });
}

// Destructor flushes and closes the file, and joins the worker thread.
ChromeWriter::~ChromeWriter() {}
void ChromeWriter::finalize() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    stop_flag_ = true;
  }
  queue_cv_.notify_one();
  if (worker_.joinable()) worker_.join();
  compressor_->compress("]");
  compressor_->finalize();
}

void ChromeWriter::push_event(EventWithId* event) {
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    event_queue_.emplace_back(event);
  }
  queue_cv_.notify_one();
}

// Serialize and write a single event to the file, including event_id as "id".
void ChromeWriter::write_event(EventWithId* event_with_id) {
  index_++;
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
      unsigned int method = 0, clazz = 0;

      if (args != nullptr) {
        if (event_with_id->event_type == COUNTER_EVENT) {
          // Handle COUNTER_EVENT specific logic
          unsigned long long duration = std::any_cast<unsigned int>((*args)["duration"]);
          if (duration > std::numeric_limits<unsigned long long>::max() / 1000) {
            duration = std::numeric_limits<unsigned long long>::max();
          } else {
            duration = static_cast<unsigned long long>(std::floor(duration / 1000.0));
          }
          (*args)["duration"] = duration;
        }
        method = std::any_cast<unsigned int>((*args)["method"]);
        clazz = std::any_cast<unsigned int>((*args)["clazz"]);
        function_name = std::to_string(clazz) + "." + std::to_string(method);
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
    int len = 0;
    if (event_with_id->event_type == COUNTER_EVENT) {
      len = std::snprintf(
          buffer, sizeof(buffer), R"({"id":%lu,"name":"%s","cat":"%s","ph":"%c","ts":%llu)", index_,
          function_name.c_str(), probe_name.c_str(), event_with_id->event_type, ts_us);
    } else if (event_with_id->event_type == METADATA_EVENT) {
      len = std::snprintf(buffer, sizeof(buffer), R"({"id":%lu,"name":"%s","cat":"%s","ph":"%c")",
                          index_, function_name.c_str(), probe_name.c_str(),
                          event_with_id->event_type);
    } else if (event_with_id->event_type == NORMAL_EVENT) {
      // Normal even
      len = std::snprintf(
          buffer, sizeof(buffer),
          R"({"id":%lu,"name":"%s","cat":"%s","ph":"%c","ts":%llu,"dur":%llu,"pid":%d,"tid":%d)",
          index_, function_name.c_str(), probe_name.c_str(), event_with_id->event_type, ts_us,
          dur_us, pid, tid);
    } else {
      return;
    }

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
      compressor_->compress(event_json);
    }
  }
  if (args != nullptr) {
    delete args;  // Clean up args after use
  }
  if (event_with_id != nullptr) {
    delete event_with_id;  // Clean up event after writing
  }
}

void ChromeWriter::worker_loop() {
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
}  // namespace datacrumbs
