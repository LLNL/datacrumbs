#include "example.generic.bpf.h"
SEC("ksyscall/openat")
int BPF_KPROBE(openat_entry, struct pt_regs* regs) {
  generic_call(1);
  return 0;
}
SEC("kretsyscall/openat")
int BPF_KRETPROBE(openat_exit, struct pt_regs* regs) {
  generic_call(2);
  return 0;
}