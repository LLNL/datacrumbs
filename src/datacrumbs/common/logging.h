#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// std headers
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

// Logging levels
#define LOG_LEVEL_PRINT 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_TRACE 5

#include <cstdarg>
#include <vector>

namespace datacrumbs::logging_internal {

inline FILE* get_log_file() {
#ifdef LOG_TO_FILE
  static FILE* file = fopen(LOG_FILE_PATH, "a");
  return file;
#else
  return stdout;
#endif
}

inline std::mutex& get_log_mutex() {
  static std::mutex mtx;
  return mtx;
}

// Default formatter: just joins messages with spaces
inline std::string default_formatter(const std::vector<std::string>& messages) {
  std::string result;
  for (size_t i = 0; i < messages.size(); ++i) {
    if (i > 0) result += " ";
    result += messages[i];
  }
  return result;
}

// Variadic template to accept any number of messages
inline void log_message_fmt(const char* level, const char* fmt, ...) {
  FILE* out = get_log_file();

  constexpr size_t buf_size = 1024;
  char buffer[buf_size];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, buf_size, fmt, args);
  va_end(args);

  fprintf(out, "[%s] %s\n", level, buffer);
  fflush(out);
}

inline void log_message_fmt_no_new_line(const char* level, const char* fmt, ...) {
  FILE* out = get_log_file();

  constexpr size_t buf_size = 1024;
  char buffer[buf_size];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, buf_size, fmt, args);
  va_end(args);

  fprintf(out, "%s", buffer);
  fflush(out);
}

// Overload for default formatter
template <typename... Args>
inline void log_message(const char* level, const char* fmt, Args&&... args) {
  logging_internal::log_message_fmt(level, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void log_message_no_new_line(const char* level, const char* fmt, Args&&... args) {
  logging_internal::log_message_fmt_no_new_line(level, fmt, std::forward<Args>(args)...);
}

// Trace-level logging with file and line info
#if DATACRUMBS_LOG_LEVEL >= LOG_LEVEL_DEBUG
inline void log_message_trace(const char* level, const char* file, int line, const char* fmt, ...) {
  FILE* out = get_log_file();

  constexpr size_t buf_size = 1024;
  char buffer[buf_size];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, buf_size, fmt, args);
  va_end(args);

  fprintf(out, "[%s] %s (%s:%d)\n", level, buffer, file, line);
  fflush(out);
}
#endif

}  // namespace datacrumbs::logging_internal

#if 1
#define DC_LOG_PRINT(...) datacrumbs::logging_internal::log_message("PRINT", __VA_ARGS__)
#define DC_LOG_PRINT_NO_NEW_LINE(...) \
  datacrumbs::logging_internal::log_message_no_new_line("PRINT", __VA_ARGS__)
#else
#define DC_LOG_PRINT(...) (void)0
#endif

// Macros for logging
#if DATACRUMBS_LOG_LEVEL >= LOG_LEVEL_ERROR
#define DC_LOG_ERROR(...) datacrumbs::logging_internal::log_message("ERROR", __VA_ARGS__)
#else
#define DC_LOG_ERROR(...) (void)0
#endif

#if DATACRUMBS_LOG_LEVEL >= LOG_LEVEL_WARN
#define DC_LOG_WARN(...) datacrumbs::logging_internal::log_message("WARN", __VA_ARGS__)
#else
#define DC_LOG_WARN(...) (void)0
#endif

#if DATACRUMBS_LOG_LEVEL >= LOG_LEVEL_INFO
#define DC_LOG_INFO(...) datacrumbs::logging_internal::log_message("INFO", __VA_ARGS__)
#else
#define DC_LOG_INFO(...) (void)0
#endif

#if DATACRUMBS_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define DC_LOG_DEBUG(...) datacrumbs::logging_internal::log_message("DEBUG", __VA_ARGS__)
#else
#define DC_LOG_DEBUG(...) (void)0
#endif

#if DATACRUMBS_LOG_LEVEL >= LOG_LEVEL_TRACE
#define DC_LOG_TRACE(...) datacrumbs::logging_internal::log_message("TRACE", __VA_ARGS__)
#else
#define DC_LOG_TRACE(...) (void)0
#endif

namespace datacrumbs::logging_internal {

inline void log_progress(const std::string& message, size_t current, size_t total) {
  using namespace std::chrono;
  static auto start_time = steady_clock::now();

  float percent = (total > 0) ? (100.0f * current / total) : 0.0f;
  auto now = steady_clock::now();
  double elapsed = duration_cast<duration<double>>(now - start_time).count();
  double rate = (elapsed > 0.0) ? (current / elapsed) : 0.0;

  DC_LOG_PRINT_NO_NEW_LINE("\r%s [%zu/%zu] %d%% completed | %.2fs elapsed | %.2f events/s",
                           message.c_str(), current, total, static_cast<int>(percent), elapsed,
                           rate);

  if (current == total) {
    DC_LOG_PRINT("%s done. Total time: %.2fs, Avg rate: %.2f events/s", message.c_str(), elapsed,
                 rate);
    start_time = steady_clock::now();  // Reset for next progress
  }
}

inline void log_progress(const std::string& message, size_t current) {
  using namespace std::chrono;
  static auto start_time = steady_clock::now();
  static std::mutex mtx;
  auto now = steady_clock::now();
  double elapsed = duration_cast<duration<double>>(now - start_time).count();
  double rate = (elapsed > 0.0) ? (current / elapsed) : 0.0;
  std::string rate_str;
  std::string multiplier;
  if (rate >= 1e9) {
    rate_str = std::to_string(rate / 1e9) + "G";
    multiplier = "G";
  } else if (rate >= 1e6) {
    rate_str = std::to_string(rate / 1e6) + "M";
    multiplier = "M";
  } else if (rate >= 1e3) {
    rate_str = std::to_string(rate / 1e3) + "K";
    multiplier = "K";
  } else {
    rate_str = std::to_string(rate);
    multiplier = "";
  }

  rate_str.resize(rate_str.find('.') != std::string::npos ? rate_str.find('.') + 3
                                                          : rate_str.size());
  rate_str += multiplier;
  int hours = static_cast<int>(elapsed) / 3600;
  int minutes = (static_cast<int>(elapsed) % 3600) / 60;
  int seconds = static_cast<int>(elapsed) % 60;
  char elapsed_str[32];
  if (hours > 0)
    snprintf(elapsed_str, sizeof(elapsed_str), "%dh %dm %ds", hours, minutes, seconds);
  else if (minutes > 0)
    snprintf(elapsed_str, sizeof(elapsed_str), "%dm %ds", minutes, seconds);
  else
    snprintf(elapsed_str, sizeof(elapsed_str), "%.2fs", elapsed);
  std::string current_str;
  if (current >= 1e9) {
    current_str = std::to_string(static_cast<double>(current) / 1e9) + "G";
    multiplier = "G";
  } else if (current >= 1e6) {
    multiplier = "M";
    current_str = std::to_string(static_cast<double>(current) / 1e6) + "M";
  } else if (current >= 1e3) {
    multiplier = "K";
    current_str = std::to_string(static_cast<double>(current) / 1e3) + "K";
  } else {
    current_str = std::to_string(current);
    multiplier = "";
  }
  current_str.resize(current_str.find('.') != std::string::npos ? current_str.find('.') + 3
                                                                : current_str.size());
  current_str += multiplier;
  DC_LOG_PRINT_NO_NEW_LINE(
      "\r                            \r%s [%s events] | %s elapsed | %s events/s    ",
      message.c_str(), current_str.c_str(), elapsed_str, rate_str.c_str());
  // Optionally, print newline if desired when called with a special value
}
}  // namespace datacrumbs::logging_internal
// Progress logging macro
#define DC_LOG_PROGRESS(message, current, total) \
  datacrumbs::logging_internal::log_progress(message, current, total)

#define DC_LOG_PROGRESS_SINGLE(message, current) \
  datacrumbs::logging_internal::log_progress(message, current)
