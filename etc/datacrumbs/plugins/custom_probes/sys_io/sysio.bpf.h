#ifndef DATACRUMBS_CUSTOM_PROBES_SYS_IO_SYSIO_BPF_H
#define DATACRUMBS_CUSTOM_PROBES_SYS_IO_SYSIO_BPF_H

struct sysio_event_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long ts;
  unsigned long long dur;
  char filename[256];
  unsigned long long size;
};

#endif  // DATACRUMBS_CUSTOM_PROBES_SYS_IO_SYSIO_BPF_H