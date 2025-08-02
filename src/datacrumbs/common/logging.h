#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

inline void log_progress(const std::string& message, size_t current, size_t total) {
  float percent = (total > 0) ? (100.0f * current / total) : 0.0f;
  std::cout << "\r" << message << " [" << current << "/" << total << "] "
            << static_cast<int>(percent) << "% completed" << std::flush;
  if (current == total) {
    std::cout << std::endl;
  }
}

// Logging levels
#define LOG_LEVEL_PRINT 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_TRACE 5

// Set default log level if not defined by CMake
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#include <cstdarg>
#include <vector>

namespace logging_internal {

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
  std::lock_guard<std::mutex> lock(get_log_mutex());
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

// Overload for default formatter
template <typename... Args>
inline void log_message(const char* level, const char* fmt, Args&&... args) {
  logging_internal::log_message_fmt(level, fmt, std::forward<Args>(args)...);
}

// Trace-level logging with file and line info
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
template <typename... Args>
inline void log_message_trace(const char* level, const char* file, int line, Args&&... args) {
  std::lock_guard<std::mutex> lock(get_log_mutex());
  FILE* out = get_log_file();

  constexpr size_t buf_size = 1024;
  char buffer[buf_size];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, buf_size, fmt, args);
  va_end(args);

  fprintf(out, "[%s] %s (%s:%d)\n", level, buffer, file, line);
  fflush(out);

#endif

}  // namespace logging_internal

#if 1
#define DC_LOG_PRINT(...) logging_internal::log_message("PRINT", __VA_ARGS__)
#else
#define DC_LOG_PRINT(...) (void)0
#endif

// Macros for logging
#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define DC_LOG_ERROR(...) logging_internal::log_message("ERROR", __VA_ARGS__)
#else
#define DC_LOG_ERROR(...) (void)0
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define DC_LOG_WARN(...) logging_internal::log_message("WARN", __VA_ARGS__)
#else
#define DC_LOG_WARN(...) (void)0
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define DC_LOG_INFO(...) logging_internal::log_message("INFO", __VA_ARGS__)
#else
#define DC_LOG_INFO(...) (void)0
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define DC_LOG_DEBUG(...) logging_internal::log_message("DEBUG", __VA_ARGS__)
#else
#define DC_LOG_DEBUG(...) (void)0
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define DC_LOG_TRACE(...) logging_internal::log_message("TRACE", __VA_ARGS__)
#else
#define DC_LOG_TRACE(...) (void)0
#endif

// Progress logging macro
#define DC_LOG_PROGRESS(message, current, total) log_progress(message, current, total)