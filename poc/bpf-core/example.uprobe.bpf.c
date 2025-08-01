
#include "example.generic.bpf.h"
SEC("uprobe//usr/lib64/libc.so.6:open")
int BPF_UPROBE(open_entry, struct pt_regs* regs) {
  generic_call(5);
  return 0;
}
SEC("uretprobe//usr/lib64/libc.so.6:open")
int BPF_URETPROBE(open_exit, struct pt_regs* regs) {
  generic_call(6);
  return 0;
}