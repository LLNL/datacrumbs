#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/usdt.bpf.h>
//
#include "example.h"
//
char message[12]= "Hello World";
// anonymous struct assigned to rb variable
struct
{
 // specify the type, eBPF specific syntax
 __uint(type, BPF_MAP_TYPE_RINGBUF);
 // specify the size of the buffer
 // has to be a multiple of the page size
 __uint(max_entries, 1024 * 1024);
} output SEC(".maps") /* placed in maps section */;
int generic_call(int event_id) {
 struct event_t *evt;
 evt= bpf_ringbuf_reserve(&output, sizeof(*evt), 0);
 if (!evt) {
  bpf_printk("Failed to reserve space in ring buffer\n");
  return 0;
 }
 unsigned long id= bpf_get_current_pid_tgid();
 int pid= id;
 evt->pid= pid;
 evt->event_id= event_id;
 bpf_ringbuf_submit(evt, 0);
 bpf_printk("Tracing function %d PID %d", event_id, pid);
 return 0;
}
SEC("ksyscall/openat")
int BPF_KPROBE(openat_entry, struct pt_regs *regs) {
 generic_call(1);
 return 0;
}
SEC("kretsyscall/openat")
int BPF_KRETPROBE(openat_exit, struct pt_regs *regs) {
 generic_call(2);
 return 0;
}
SEC("kprobe/vfs_write")
int BPF_KPROBE(vfs_write_entry) {
 generic_call(3);
 return 0;
}
SEC("kretprobe/vfs_write")
int BPF_KRETPROBE(vfs_write_exit, struct pt_regs *regs) {
 generic_call(4);
 return 0;
}
SEC("uprobe//usr/lib64/libc.so.6:open")
int BPF_UPROBE(open_entry, struct pt_regs *regs) {
 generic_call(5);
 return 0;
}
SEC("uretprobe//usr/lib64/libc.so.6:open")
int BPF_URETPROBE(open_exit, struct pt_regs *regs) {
 generic_call(6);
 return 0;
}
SEC("usdt//usr/lib64/libpython3.11.so:python:function__entry")
int BPF_USDT(python_function_entry, void *class, void *function) {
 generic_call(7);
 return 0;
}
SEC("usdt//usr/lib64/libpython3.11.so:python:function__return")
int BPF_USDT(python_function_return, void *class, void *function) {
 generic_call(8);
 return 0;
}
char LICENSE[] SEC("license")= "Dual BSD/GPL";