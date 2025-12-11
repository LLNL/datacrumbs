#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/.bpf-build"
BPF_SRC="${SCRIPT_DIR}/tracepoint_bpf.c"
VMLINUX_H="${BUILD_DIR}/vmlinux.h"
BPF_OBJ="${BUILD_DIR}/tracepoint_bpf.o"
PROG_PIN="${BUILD_DIR}/tracepoint_prog"

mkdir -p "${BUILD_DIR}"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

require_cmd clang
require_cmd bpftool

if [[ ! -s "${VMLINUX_H}" ]]; then
  echo "Generating vmlinux.h from running kernel BTF..."
  bpftool btf dump file /sys/kernel/btf/vmlinux format c > "${VMLINUX_H}"
fi

CFLAGS=(
  -target bpf
  -g -O2
  -D__TARGET_ARCH_x86
  -I"${BUILD_DIR}"
  -I/usr/include
  -I/usr/include/bpf
)

echo "Compiling ${BPF_SRC} -> ${BPF_OBJ}"
clang "${CFLAGS[@]}" -c "${BPF_SRC}" -o "${BPF_OBJ}"

echo "Loading BPF program via bpftool (pinning at ${PROG_PIN})"
if ! ulimit -l unlimited 2>/dev/null; then
  CUR_LOCK_LIMIT="$(ulimit -l)"
  echo "Warning: unable to set unlimited memlock (current limit: ${CUR_LOCK_LIMIT} KB); continuing with existing limit."
fi
bpftool prog load "${BPF_OBJ}" "${PROG_PIN}"

echo "Attaching program to kprobe/sys_getpid"
bpftool prog attach pinned "${PROG_PIN}" kprobe/sys_getpid

cat <<'NOTE'

Program loaded and attached. In another terminal run:
  cat /sys/kernel/debug/tracing/trace_pipe
and trigger sys_getpid (e.g. run `ps`) to see bpf_printk output.

Detach/cleanup when done:
  bpftool prog detach pinned BUILD_DIR/tracepoint_prog kprobe/sys_getpid
  rm BUILD_DIR/tracepoint_prog

NOTE
