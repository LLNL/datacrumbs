#pragma once

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <datacrumbs/bpf/datacrumbs.skel.h>
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

int initialize_map_2(struct datacrumbs_bpf* skel) {
  int profile_map_fd = bpf_map__fd(skel->maps.sysio_profile);
  if (profile_map_fd < 0) {
    DC_LOG_ERROR("Failed to get sysio profile map fd: %d", profile_map_fd);
    datacrumbs_bpf__destroy(skel);
    return -1;
  } else {
    DC_LOG_DEBUG("Successful opening of sysio profile");
  }
  return profile_map_fd;
}

inline static int lookup_2(int map_fd, unsigned long long latest_timestamp,
                           datacrumbs::EventProcessor* event_processor, unsigned int batch_size) {
  static struct sysio_counter_key_t* sysio_keys = NULL;
  static struct sysio_counter_value_t* sysio_values = NULL;
  static struct sysio_counter_key_t* sysio_in_batch = NULL;
  if (sysio_keys == NULL || sysio_values == NULL || sysio_in_batch == NULL) {
    DC_LOG_INFO("Creating new datastructure for sysio");
    sysio_keys = new sysio_counter_key_t[batch_size];
    sysio_values = new sysio_counter_value_t[batch_size];
  }
  int ret = -1;
  if (latest_timestamp == 0) {
    ret = bpf_map_lookup_and_delete_batch(map_fd, sysio_in_batch, &sysio_in_batch, sysio_keys,
                                          sysio_values, &batch_size, 0);
  } else {
    ret = bpf_map_lookup_batch(map_fd, sysio_in_batch, &sysio_in_batch, sysio_keys, sysio_values,
                               &batch_size, 0);
  }
  if (ret < 0 && errno != ENOENT) {
    DC_LOG_ERROR("bpf_map_lookup_batch for sysio map %d", ret);
    perror("bpf_map_lookup_batch");
    return -1;
  } else if (ret < 0) {
    DC_LOG_ERROR("bpf_map_lookup_batch for sysio map:%d ret:%d, errno: %d (%s)", map_fd, ret, errno,
                 strerror(errno));
    return -1;
  }
  struct sysio_counter_key_t delete_keys[batch_size];
  unsigned int j = 0;
  // Process the retrieved keys and values
  for (int i = 0; i < batch_size; ++i) {
    if (latest_timestamp == 0 || sysio_keys[i].time_interval <= latest_timestamp) {
      struct sysio_counter_event_t event;
      event.key = &sysio_keys[i];
      event.value = &sysio_values[i];
      event_processor->handle_event(&event, 1024);
      delete_keys[j++] = sysio_keys[i];
    }
  }
  if (latest_timestamp == 0) {
    ret = bpf_map_delete_batch(map_fd, delete_keys, &j, NULL);
    if (ret < 0) {
      DC_LOG_ERROR("bpf_map_delete_batch for sysio map %d", ret);
      perror("bpf_map_delete_batch");
    }
  }
  // Check if the end of the map has been reached
  if (ret < 0 && errno == ENOENT) {
    DC_LOG_ERROR("bpf_map_lookup_batch for sysio map %d", ret);
    return -1;
  }
  return 0;
}

#endif