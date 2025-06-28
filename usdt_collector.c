
#include <linux/bpf.h>
#include <linux/sched.h>
#include <uapi/linux/limits.h>
#include <uapi/linux/ptrace.h>
struct fn_key_t {
 u64 ip;
 s64 pid;
};
struct fn_t {
 u64 ts;
};
struct file_t {
 u64 id;
 int fd;
};
struct filename_t {
 char fname[256];
};
#define MAX_STRING_LENGTH 80
BPF_HASH(pid_map, u32, u64);                        // map for apps to collect data
BPF_HASH(fn_pid_map, struct fn_key_t, struct fn_t); // collect start time and ip for apps
BPF_HASH(file_hash, u64, struct filename_t, 10240);
BPF_HASH(latest_hash, struct fn_key_t, u64);
BPF_HASH(latest_fd, u64, int);
BPF_HASH(fd_hash, struct file_t, u64);
BPF_HASH(pid_hash, u64, u64);
BPF_RINGBUF_OUTPUT(events, 1 << 16); // emit events to python
struct usdt_python_t {
 u64 id;
 u64 event_id;
 u64 ts;
 u64 dur;
 char clazz[MAX_STRING_LENGTH];
 char method[MAX_STRING_LENGTH];
};
int trace_python_entry(struct pt_regs *ctx) {
 u64 id= bpf_get_current_pid_tgid();
 u32 pid= id;
 u64 start_ts= USDT_START_TS;
 if (pid != USDT_PID)
  return 0;
 struct fn_key_t key= {};
 key.pid= pid;
 key.ip= 100001;
 struct fn_t fn= {};
 fn.ts= bpf_ktime_get_ns();
 fn_pid_map.update(&key, &fn);
 return 0;
}
int trace_python_exit(struct pt_regs *ctx) {
 u64 id= bpf_get_current_pid_tgid();
 u32 pid= id;
 u64 start_ts= USDT_START_TS;
 if (pid != USDT_PID)
  return 0;
 struct fn_key_t key= {};
 key.pid= pid;
 key.ip= 100001;
 struct fn_t *fn= fn_pid_map.lookup(&key);
 if (fn == 0)
  return 0; // missed entry
 struct usdt_python_t stats_key_v= {};
 struct usdt_python_t *stats_key= &stats_key_v;
 u64 clazz= 0, method= 0;
 bpf_usdt_readarg(1, ctx, &clazz);
 bpf_usdt_readarg(2, ctx, &method);
 bpf_probe_read_user(&stats_key->clazz, sizeof(stats_key->clazz),
                     (void *)clazz);
 bpf_probe_read_user(&stats_key->method, sizeof(stats_key->method),
                     (void *)method);
 stats_key->id= id;
 stats_key->ts= (fn->ts - start_ts);
 stats_key->event_id= 100001;
 stats_key->dur= bpf_ktime_get_ns() - fn->ts;
 bpf_trace_printk("Tracing USDT PID \%d \%s \%d", pid, stats_key->method, stats_key->event_id);
 events.ringbuf_output(stats_key, sizeof(struct usdt_python_t), 0);
 return 0;
}
