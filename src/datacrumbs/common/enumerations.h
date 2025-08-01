#pragma once

#include <stdexcept>
#include <string>
namespace datacrumbs {

enum class Mode : uint8_t {
  PROFILER = 0,
  TRACER = 1,
};

inline void convert(const std::string& s, Mode& type) {
  if (s == "profiler") {
    type = Mode::PROFILER;
  } else if (s == "tracer") {
    type = Mode::TRACER;
  } else {
    throw std::invalid_argument("Unknown Mode: " + s + ". Valid types are: profiler or tracer.");
  }
}

enum class ProbeType : uint8_t {
  SYSCALLS = 0,
  KPROBE = 1,
  UPROBE = 2,
  USDT = 3,
};

inline void convert(const std::string& s, ProbeType& type) {
  if (s == "syscalls") {
    type = ProbeType::SYSCALLS;
  } else if (s == "kprobe") {
    type = ProbeType::KPROBE;
  } else if (s == "uprobe") {
    type = ProbeType::UPROBE;
  } else if (s == "usdt") {
    type = ProbeType::USDT;
  } else {
    throw std::invalid_argument("Unknown ProbeType: " + s +
                                ". Valid types are: syscalls, kprobe, uprobe, or usdt.");
  }
}

enum class CaptureType : uint8_t {
  HEADER = 0,
  BINARY = 1,
  KSYM = 2,
};

inline void convert(const std::string& s, CaptureType& type) {
  if (s == "header") {
    type = CaptureType::HEADER;
  } else if (s == "binary") {
    type = CaptureType::BINARY;
  } else if (s == "ksym") {
    type = CaptureType::KSYM;
  } else {
    throw std::invalid_argument("Unknown CaptureType: " + s +
                                ". Valid types are: header, binary, or ksym.");
  }
}

}  // namespace datacrumbs
