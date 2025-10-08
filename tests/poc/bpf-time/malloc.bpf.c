// malloc.bpf.c

#include "vmlinux.h"
//
#include <bpf/bpf_core_read.h>
//
#include <bpf/bpf_helpers.h>
//
#include <bpf/bpf_tracing.h>
//
#include <bpf/usdt.bpf.h>
//

// BPF map to store the count of malloc calls
struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, __u64);
} counts SEC(".maps");

SEC("uprobe//usr/lib64/libc.so.6:malloc")
int BPF_UPROBE(malloc_probe, struct pt_regs* regs) {
  __u32 key = 0;
  __u64* count = bpf_map_lookup_elem(&counts, &key);
  if (count) {
    (*count)++;
  }
  return 0;
}
