#pragma once

#include <datacrumbs/common/typdefs.h>

#include <cstdint>

#include "sysio.bpf.h"

#define GET_DATA_2_EXISTS

DataCrumbsArgs get_data_2(void* data) {
  struct sysio_event_t* event = (struct sysio_event_t*)data;
  DataCrumbsArgs args;
  args.emplace_back(std::pair<std::string, uint64_t>(std::string("fhash"), event->file_hash));
  args.emplace_back(std::pair<std::string, uint64_t>(std::string("size"), event->size));
  return args;
}