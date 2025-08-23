//
// Created by haridev on 3/28/23.
//

#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/logging.h>

// std headers
#include <memory>
#include <utility>

namespace datacrumbs {

/**
 * @brief Singleton class template to ensure a single instance of T.
 *
 * Uses a shared pointer to manage the instance. Logging is provided using
 * printf-style macros from datacrumbs/common/logging.h.
 */
template <typename T>
class Singleton {
 public:
  /**
   * @brief Get the singleton instance of T.
   *
   * If instance creation is stopped, returns nullptr.
   * Logs function entry/exit and instance creation.
   */
  template <typename... Args>
  static std::shared_ptr<T> get_instance(Args... args) {
    DC_LOG_TRACE("Entering Singleton::get_instance");
    if (stop_creating_instances) {
      DC_LOG_WARN("Attempted to get instance after finalization");
      DC_LOG_TRACE("Exiting Singleton::get_instance");
      return nullptr;
    }
    if (instance == nullptr) {
      DC_LOG_DEBUG("Creating new instance of Singleton<%s>", typeid(T).name());
      instance = std::make_shared<T>(std::forward<Args>(args)...);
    } else {
      DC_LOG_DEBUG("Returning existing instance of Singleton<%s>", typeid(T).name());
    }
    DC_LOG_TRACE("Exiting Singleton::get_instance");
    return instance;
  }

  /**
   * @brief Deleted assignment operator.
   */
  Singleton& operator=(const Singleton) = delete;

  /**
   * @brief Deleted copy constructor.
   */
  Singleton(const Singleton&) = delete;

  /**
   * @brief Finalize the singleton, preventing further instance creation.
   *
   * Logs the finalization event.
   */
  static void finalize() {
    DC_LOG_INFO("Finalizing Singleton<%s>, no further instances will be created", typeid(T).name());
    stop_creating_instances = true;
  }

 protected:
  static bool stop_creating_instances;  ///< Flag to stop instance creation
  static std::shared_ptr<T> instance;   ///< Singleton instance

  /**
   * @brief Hidden default constructor.
   */
  Singleton() {}
};

}  // namespace datacrumbs
