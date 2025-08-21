#ifndef DATACRUMBS_CUSTOM_PROBES_SYS_IO_SYSIO_BPF_H
#define DATACRUMBS_CUSTOM_PROBES_SYS_IO_SYSIO_BPF_H

#include <datacrumbs/server/bpf/shared.h>

#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 1)
struct sysio_event_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long ts;
  unsigned long long dur;
  unsigned int fhash;
  unsigned long long size;
};
#else
struct sysio_counter_key_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long time_interval;
  unsigned int fhash;
};

struct sysio_counter_value_t {
  unsigned long long duration;
  unsigned long long frequency;
  unsigned long long size;
};

struct sysio_counter_event_t {
  struct sysio_counter_key_t* key;
  struct sysio_counter_value_t* value;
};
#endif

#endif  // DATACRUMBS_CUSTOM_PROBES_SYS_IO_SYSIO_BPF_H