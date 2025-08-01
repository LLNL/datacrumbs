
SEC("kprobe/vfs_write")
int BPF_KPROBE(vfs_write_entry) {
  generic_call(3);
  return 0;
}
SEC("kretprobe/vfs_write")
int BPF_KRETPROBE(vfs_write_exit, struct pt_regs* regs) {
  generic_call(4);
  return 0;
}