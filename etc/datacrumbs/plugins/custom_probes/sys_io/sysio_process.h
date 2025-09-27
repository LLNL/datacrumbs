#pragma once

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <datacrumbs/bpf/datacrumbs.skel.h>
#include <datacrumbs/common/constants.h>
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/typedefs.h>
#include <datacrumbs/server/process/event_processor.h>

#include <cstdint>
#include <string>

#include "sysio.bpf.h"

#define GET_DATA_2_EXISTS

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
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
#else

datacrumbs::EventWithId* get_data_2(void* data, uint64_t index) {
  struct sysio_counter_event_t* base = (struct sysio_counter_event_t*)data;
  auto args = new DataCrumbsArgs();
  if (base->key->fhash != 0) {
    args->emplace("fhash", base->key->fhash);
  }
  if (base->value->size != 0) {
    args->emplace("size", base->value->size);
  }
  auto event = new datacrumbs::EventWithId(COUNTER_EVENT, index, base->key->type, base->key->id,
                                           base->key->event_id, base->key->time_interval, 0, args);
  return event;
}

#define INITIALIZE_MAP_2()                                                \
  int profile_2_fd = bpf_map__fd(skel->maps.sysio_profile);               \
  if (profile_2_fd < 0) {                                                 \
    DC_LOG_ERROR("Failed to get sysio profile map fd: %d", profile_2_fd); \
    datacrumbs_bpf__destroy(skel);                                        \
    return 1;                                                             \
  }

#define INITIALIZE_MAP_LOOKUP_2()                                                               \
  struct sysio_counter_key_t* sysio_counter_keys =                                              \
      (struct sysio_counter_key_t*)malloc(batch_size * sizeof(struct sysio_counter_key_t));     \
  struct sysio_counter_value_t* sysio_counter_values =                                          \
      (struct sysio_counter_value_t*)malloc(batch_size * sizeof(struct sysio_counter_value_t)); \
  struct sysio_counter_key_t* sysio_counter_in_batch = nullptr;

#define LOOKUP_2_CALL()                                                               \
  lookup_2(profile_2_fd, latest_ts, &event_processor, batch_size, sysio_counter_keys, \
           sysio_counter_values, sysio_counter_in_batch)

inline static int lookup_2(int map_fd, unsigned long long latest_timestamp,
                           datacrumbs::EventProcessor* event_processor, unsigned int batch_size,
                           struct sysio_counter_key_t* keys, struct sysio_counter_value_t* values,
                           struct sysio_counter_key_t* in_batch) {
  int ret = bpf_map_lookup_batch(map_fd, in_batch, &in_batch, keys, values, &batch_size, 0);
  if (ret < 0 && errno != ENOENT) {
    perror("bpf_map_lookup_batch sysio");
    return -1;
  }
  if (batch_size < 1) {
    return -1;
  }
  struct sysio_counter_key_t delete_keys[batch_size];
  unsigned int j = 0;
  // Process the retrieved keys and values
  for (int i = 0; i < batch_size; ++i) {
    if (latest_timestamp == 0 || keys[i].time_interval <= latest_timestamp) {
      struct sysio_counter_event_t event;
      event.key = &keys[i];
      event.value = &values[i];
      event_processor->handle_event(&event, 1024);
      delete_keys[j++] = keys[i];
    }
  }
  ret = bpf_map_delete_batch(map_fd, delete_keys, &j, NULL);
  if (ret < 0) {
    perror("bpf_map_delete_batch sysio");
  }
  // Check if the end of the map has been reached
  if (ret < 0 && errno == ENOENT) {
    return -1;
  }
  return 0;
}

#endif