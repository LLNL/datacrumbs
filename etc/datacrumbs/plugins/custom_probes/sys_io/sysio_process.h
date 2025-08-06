#pragma once

#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/typedefs.h>

#include <cstdint>
#include <string>

#include "sysio.bpf.h"

#define GET_DATA_2_EXISTS

datacrumbs::EventWithId* get_data_2(void* data, uint64_t index) {
  struct sysio_event_t* base = (struct sysio_event_t*)data;
  auto args = new DataCrumbsArgs();
  if (base->fhash != 0) {
    args->emplace("fhash", base->fhash);
  }
  if (base->size != 0) {
    args->emplace("size", base->size);
  }
  auto event = new datacrumbs::EventWithId(NORMAL_EVENT, index, base->type, base->id,
                                           base->event_id, base->ts, base->dur, args);
  return event;
}