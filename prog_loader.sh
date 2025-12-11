#!/usr/bin/env bash
set -euo pipefail
ulimit -l unlimited
bpftool prog load ./prog.o /sys/fs/bpf/tracepoint_prog
bpftool prog attach pinned /sys/fs/bpf/tracepoint_prog kprobe/sys_getpid
