#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

SEC("kprobe/sys_getpid")
int handle_getpid(struct pt_regs *ctx)
{
    bpf_printk("hello from eBPF\n");
    return 0;
}
