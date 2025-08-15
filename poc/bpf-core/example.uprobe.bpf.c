
#include "example.generic.bpf.h"
SEC("uprobe//workspaces/datacrumbs/build/lib/libdatacrumbs_client.so:datacrumbs_start")
int BPF_UPROBE(open_entry, struct pt_regs* regs) {
  generic_call(5);
  return 0;
}
SEC("uretprobe//workspaces/datacrumbs/build/lib/libdatacrumbs_client.so:datacrumbs_stop")
int BPF_URETPROBE(open_exit, struct pt_regs* regs) {
  generic_call(6);
  return 0;
}