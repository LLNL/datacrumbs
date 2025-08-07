#include <datacrumbs/server/bpf/common.h>

#ifndef DATACRUMBS_BUILD_CLIENT_SO
#error "DATACRUMBS_BUILD_CLIENT_SO must be defined"
#pragma message("DATACRUMBS_BUILD_CLIENT_SO = " STR(DATACRUMBS_BUILD_CLIENT_SO))
#endif

#ifndef DATACRUMBS_LIBC_SO
#define DATACRUMBS_LIBC_SO "/lib64/libc.so.6"
#endif

static inline __attribute__((always_inline)) int generic_trace_datacrumbs_start() {
  u64 tsp = bpf_ktime_get_ns();
  u64 id = bpf_get_current_pid_tgid();
  u32 pid = id & 0xFFFFFFFF;
  u64* start_ts = bpf_map_lookup_elem(&pid_map, &pid);
  if (start_ts != 0) tsp = *start_ts;
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

static inline __attribute__((always_inline)) int generic_fork_exit(struct pt_regs* ctx,
                                                                   u64 event_id) {
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (need_tracing(&key, &start_ts)) {
    // u64 id = bpf_get_current_pid_tgid();
    u64 tsp = bpf_ktime_get_ns();
    u32 pid = PT_REGS_RC(ctx);
    (void)pid;
    if (pid != 0) {
      DBG_PRINTK("Collect forked tracing PID %d", pid);
      bpf_map_update_elem(&pid_map, &pid, &tsp, BPF_ANY);
    }
  }
  return generic_exit(ctx, event_id);
}

SEC("ksyscall/fork")
int BPF_KSYSCALL(fork_entry, struct pt_regs* regs) {
  return generic_entry(ctx, 100);
}

SEC("kretsyscall/fork")
int BPF_KRETPROBE(fork_exit, struct pt_regs* regs) {
  return generic_fork_exit(ctx, 100);
}

SEC("ksyscall/vfork")
int BPF_KSYSCALL(vfork_entry, struct pt_regs* regs) {
  return generic_entry(ctx, 101);
}
SEC("kretsyscall/vfork")
int BPF_KRETPROBE(vfork_exit, struct pt_regs* regs) {
  return generic_fork_exit(ctx, 101);
}

#ifdef __GI___fork
#define GI_FORK "uprobe/" STR(DATACRUMBS_LIBC_SO) ":__GI___fork"
SEC(GI_FORK)
int BPF_UPROBE(__GI___fork_entry) {
  return generic_entry(ctx, 102);
}

#define GI_FORK_RET "uretprobe/" STR(DATACRUMBS_LIBC_SO) ":__GI___fork"
SEC(GI_FORK_RET)
int BPF_URETPROBE(__GI___fork_exit) {
  return generic_fork_exit(ctx, 102);
}
#define GI_VFORK "uprobe/" STR(DATACRUMBS_LIBC_SO) ":__GI___vfork"
SEC(GI_VFORK)
int BPF_UPROBE(__GI___vfork_entry) {
  return generic_entry(ctx, 103);
}
#define GI_VFORK_RET "uretprobe/" STR(DATACRUMBS_LIBC_SO) ":__GI___vfork"
SEC(GI_VFORK_RET)
int BPF_URETPROBE(__GI___vfork_exit) {
  return generic_fork_exit(ctx, 103);
}
#endif