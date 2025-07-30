#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
//
#include "example.h"
//
char message[12]= "Hello World";
struct {
 __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
 __uint(key_size, sizeof(u32));
 __uint(value_size, sizeof(u32));
} output SEC(".maps");
struct user_msg_t {
 char message[12];
};
struct {
 __uint(type, BPF_MAP_TYPE_HASH);
 __uint(max_entries, 10240);
 __type(key, u32);
 __type(value, struct user_msg_t);
} my_config SEC(".maps");
SEC("ksyscall/execve")
int BPF_KPROBE(hello, struct pt_regs *regs) {
 u32 pid= bpf_get_current_pid_tgid() >> 32;
 bpf_trace_printk("Tracing function enter %s PID %d", pid);
 //  struct data_t data= {};
 //  struct user_msg_t *p;
 //  data.pid= pid;
 //  data.uid= bpf_get_current_uid_gid() & 0xFFFFFFFF;
 //  bpf_get_current_comm(&data.command, sizeof(data.command));
 //  bpf_probe_read_user_str(&data.path, sizeof(data.path), pathname);
 //  p= bpf_map_lookup_elem(&my_config, &data.uid);
 //  if (p != 0) {
 //   bpf_probe_read_kernel_str(&data.message, sizeof(data.message), p->message);
 //  } else {
 //   bpf_probe_read_kernel_str(&data.message, sizeof(data.message), message);
 //  }
 //  bpf_perf_event_output(ctx, &output, BPF_F_CURRENT_CPU, &data, sizeof(data));
 return 0;
}
SEC("kretsyscall/execve")
int BPF_KRETPROBE(hello3, struct pt_regs *regs) {
 u32 pid= bpf_get_current_pid_tgid() >> 32;
 bpf_trace_printk("Tracing execve exit PID %d", pid);
 return 0;
}
SEC("kprobe/write_inode_now")
int BPF_KPROBE(write_inode_now2) {
 u32 pid= bpf_get_current_pid_tgid() >> 32;
 bpf_trace_printk("Tracing write_inode entry PID %d", pid);
 return 0;
}
SEC("kretprobe/write_inode_now")
int BPF_KRETPROBE(write_inode_now3, struct pt_regs *regs) {
 u32 pid= bpf_get_current_pid_tgid() >> 32;
 bpf_trace_printk("Tracing write_inode exit PID %d", pid);
 return 0;
}
SEC("uprobe/usr/lib64/libc.so.6:printf")
int BPF_UPROBE(printf23, struct pt_regs *regs) {
 u32 pid= bpf_get_current_pid_tgid() >> 32;
 bpf_trace_printk("Tracing printf entry PID %d", pid);
 return 0;
}
SEC("uretprobe/usr/lib64/libc.so.6:printf")
int BPF_URETPROBE(printf2, struct pt_regs *regs) {
 u32 pid= bpf_get_current_pid_tgid() >> 32;
 bpf_trace_printk("Tracing printf exit PID %d", pid);
 return 0;
}
char LICENSE[] SEC("license")= "Dual BSD/GPL";