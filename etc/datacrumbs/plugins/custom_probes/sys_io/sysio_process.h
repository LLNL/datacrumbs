#pragma once

#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/typdefs.h>

#include <cstdint>
#include <string>

#include "sysio.bpf.h"

#define GET_DATA_2_EXISTS

datacrumbs::EventWithId* get_data_2(void* data, uint64_t index) {
  struct sysio_event_t* base = (struct sysio_event_t*)data;
  auto args = new DataCrumbsArgs();
  if (base->filename[0] != '\0') {
    args->emplace("fname", std::string(base->filename));
    DC_LOG_DEBUG("Arg: fname = %s, type = %u, event_id = %llu", base->filename, base->type,
                 base->event_id);
  }
  if (base->size != 0) {
    args->emplace("size", base->size);
    DC_LOG_DEBUG("Arg: size = %llu, type = %u, event_id = %llu", base->size, base->type,
                 base->event_id);
  }
  auto event = new datacrumbs::EventWithId(index, base->type, base->id, base->event_id, base->ts,
                                           base->dur, args);
  return event;
}