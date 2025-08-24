#include <datacrumbs/server/bpf/common.h>

static inline __attribute__((always_inline)) int generic_trace_datacrumbs_start() {
  u64 tsp = bpf_ktime_get_ns();
  u64 id = bpf_get_current_pid_tgid();
  u32 pid = id & 0xFFFFFFFF;
  u64* start_ts = bpf_map_lookup_elem(&pid_map, &pid);
  if (start_ts != 0) tsp = *start_ts;
#if defined(DATACRUMBS_MODE) && (DATACRUMBS_MODE == 2)
  unsigned long long interval = tsp / DATACRUMBS_TIME_INTERVAL_NS;
  bpf_map_update_elem(&latest_interval, &DATACRUMBS_TS_KEY, &interval, BPF_ANY);
#endif
  bpf_map_update_elem(&pid_map, &pid, &tsp, BPF_ANY);
  (void)pid;
  DBG_PRINTK("Tracing PID %d", pid);
  return 0;
}
static inline __attribute__((always_inline)) int generic_trace_datacrumbs_stop() {
  u64 id = bpf_get_current_pid_tgid();
  u32 pid = id & 0xFFFFFFFF;
  (void)pid;
  DBG_PRINTK("Stop tracing PID %d", pid);
  bpf_map_delete_elem(&pid_map, &pid);
  return 0;
}

#define DATACRUMBS_START "uprobe/" DATACRUMBS_BUILD_CLIENT_SO ":datacrumbs_start"
SEC((DATACRUMBS_START))
int BPF_UPROBE(trace_datacrumbs_start) {
  return generic_trace_datacrumbs_start();
}

#define DATACRUMBS_STOP "uprobe/" DATACRUMBS_BUILD_CLIENT_SO ":datacrumbs_stop"
SEC((DATACRUMBS_STOP))
int BPF_UPROBE(trace_datacrumbs_stop) {
  return generic_trace_datacrumbs_stop();
}