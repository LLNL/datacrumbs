#include <datacrumbs/server/bpf/common.h>

#ifndef DATACRUMBS_BUILD_CLIENT_SO
#error "DATACRUMBS_BUILD_CLIENT_SO must be defined"
#pragma message("DATACRUMBS_BUILD_CLIENT_SO = " STR(DATACRUMBS_BUILD_CLIENT_SO))
#endif

static inline __attribute__((always_inline)) int generic_trace_datacrumbs_start() {
  u64 id = bpf_get_current_pid_tgid();
  u64* start_ts = bpf_map_lookup_elem(&pid_map, &id);
  u64 tsp = bpf_ktime_get_ns();
  if (start_ts != 0) tsp = *start_ts;
  bpf_map_update_elem(&pid_map, &id, &tsp, BPF_ANY);
  u32 pid = id;
  (void)pid;
  DBG_PRINTK("Tracing PID %d", pid);
  return 0;
}
static inline __attribute__((always_inline)) int generic_trace_datacrumbs_stop() {
  u64 id = bpf_get_current_pid_tgid();
  u32 pid = id;
  (void)pid;
  DBG_PRINTK("Stop tracing PID %d", pid);
  bpf_map_delete_elem(&pid_map, &id);
  return 0;
}

#define DATACRUMBS_START "uprobe/" STR(DATACRUMBS_BUILD_CLIENT_SO) ":datacrumbs_start"
SEC((DATACRUMBS_START))
int BPF_UPROBE(trace_datacrumbs_start) {
  return generic_trace_datacrumbs_start();
}

#define DATACRUMBS_STOP "uprobe/" STR(DATACRUMBS_BUILD_CLIENT_SO) ":datacrumbs_stop"
SEC((DATACRUMBS_STOP))
int BPF_UPROBE(trace_datacrumbs_stop) {
  return generic_trace_datacrumbs_stop();
}
