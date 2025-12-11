#!/usr/bin/env bash
# Setup script to install eBPF dependencies and test environment inside the VM
set -euo pipefail

# Avoid interactive apt prompts
export DEBIAN_FRONTEND=noninteractive

# Optional arguments
WORKSPACE_DIR=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --workspace)
            [[ $# -lt 2 ]] && { echo "Missing value for --workspace" >&2; exit 1; }
            WORKSPACE_DIR="$2"
            shift 2
            ;;
        *)
            # Ignore unknown args for forward compatibility
            shift
            ;;
    esac
done

DEFAULT_LOG="$HOME/datacrumbs-setup.log"
# If a workspace was provided, place log inside the repo's vm folder
if [[ -n "$WORKSPACE_DIR" ]] && [[ -d "$WORKSPACE_DIR" ]]; then
    LOG_DIR="$WORKSPACE_DIR/infrastructure/qemu/vm"
    mkdir -p "$LOG_DIR"
    LOG_FILE="$LOG_DIR/datacrumbs-setup.log"
else
    LOG_FILE="$DEFAULT_LOG"
fi
{
echo "=========================================="
echo "DataCrumbs eBPF Development Environment"
echo "=========================================="
echo ""

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo "Please run this script as the ubuntu user (not root)" 
   exit 1
fi

TARGET_DIR=""
if [[ -n "$WORKSPACE_DIR" ]] && [[ -d "$WORKSPACE_DIR" ]]; then
    TARGET_DIR="$WORKSPACE_DIR"
    echo "✓ Using provided workspace: $TARGET_DIR"
elif [[ -d /mnt/datacrumbs ]]; then
    TARGET_DIR="/mnt/datacrumbs"
    echo "✓ Workspace mounted at $TARGET_DIR"
else
    echo "Workspace mount not detected; proceeding with local copy."
    TARGET_DIR="$HOME/datacrumbs"
    mkdir -p "$TARGET_DIR"
    echo "✓ Using local workspace directory: $TARGET_DIR"
fi

echo ""
echo "Installing system dependencies..."
sudo apt-get update -qq

# Install kernel headers first (needed for linux/bpf.h)
echo "Installing kernel headers..."
KERNEL_VERSION=$(uname -r)
sudo apt-get install -y -o Dpkg::Options::="--force-confnew" linux-headers-${KERNEL_VERSION} linux-headers-generic

# Install eBPF development tools
echo "Installing eBPF tools..."
sudo apt-get install -y -o Dpkg::Options::="--force-confnew" \
    build-essential \
    clang llvm \
    libelf-dev libpcap-dev libbfd-dev binutils-dev \
    pkg-config zlib1g-dev libssl-dev libcap-dev \
    cmake \
    git \
    python3-pip \
    vim

echo "✓ eBPF tools installed"

# Install additional dependencies
echo "Installing datacrumbs dependencies..."
sudo apt-get install -y -o Dpkg::Options::="--force-confnew" \
    libbpf-dev \
    libyaml-cpp-dev \
    libopenmpi-dev \
    openmpi-bin

echo "✓ DataCrumbs dependencies installed"

echo ""
echo "=========================================="
echo "Testing eBPF Environment"
echo "=========================================="

echo ""
echo "Building libbpf (v1.5.0) and bpftool (v7.5.0) from source..."
SRC_DIR="$HOME/src-ebpf"
mkdir -p "$SRC_DIR"
if ! command -v bpftool >/dev/null 2>&1; then
    (
        set -e
        cd "$SRC_DIR"
        if [[ ! -d bpftool ]]; then
            git clone --recurse-submodules https://github.com/libbpf/bpftool.git
        fi
        cd bpftool
        git fetch --tags || true
        git checkout tags/v7.5.0 || true
        cd libbpf
        git fetch --tags || true
        git checkout tags/v1.5.0 || true
        cd src && make -j && sudo make install
        cd ../../src && make -j && sudo make install
    ) && echo "✓ Installed libbpf v1.5.0 and bpftool v7.5.0" || echo "⚠ Failed to build/install bpftool/libbpf"
else
    echo "bpftool already installed; skipping source build"
fi

# Test 1: Check BPF system call availability
echo ""
echo "Test 1: Checking BPF system call..."
if sudo bpftool feature 2>&1 | grep -q "eBPF"; then
    echo "✓ BPF system call available"
else
    echo "✗ BPF system call not available"
fi

# Test 2: Check unprivileged BPF
echo ""
echo "Test 2: Checking BPF privileges..."
UNPRIV=$(cat /proc/sys/kernel/unprivileged_bpf_disabled 2>/dev/null || echo "unknown")
if [[ "$UNPRIV" == "0" ]]; then
    echo "✓ Unprivileged BPF is enabled"
elif [[ "$UNPRIV" == "1" ]]; then
    echo "⚠ Unprivileged BPF is disabled (you'll need sudo for BPF programs)"
else
    echo "⚠ Cannot determine BPF privilege status"
fi

# Test 3: Check BTF (BPF Type Format)
echo ""
echo "Test 3: Checking BTF support..."
if [[ -f /sys/kernel/btf/vmlinux ]]; then
    echo "✓ BTF vmlinux available at /sys/kernel/btf/vmlinux"
else
    echo "✗ BTF vmlinux not found"
fi

# Test 4: Check kernel version
echo ""
echo "Test 4: Kernel version..."
KERNEL_VERSION=$(uname -r)
echo "  Kernel: $KERNEL_VERSION"

# Test 5: Test clang/LLVM
echo ""
echo "Test 5: Checking clang/LLVM..."
CLANG_VERSION=$(clang --version | head -1)
LLVM_VERSION=$(llvm-objdump --version | head -1)
echo "  $CLANG_VERSION"
echo "  $LLVM_VERSION"
echo "✓ Clang and LLVM are available"

# Test 6: Simple BPF program test (use vmlinux.h to avoid noisy kernel header includes)
echo ""
echo "Test 6: Compiling test BPF program..."
TEST_BPF_DIR=$(mktemp -d)
cd "$TEST_BPF_DIR"

# Generate vmlinux.h from BTF if available (preferred header set for BPF)
if [[ -f /sys/kernel/btf/vmlinux ]]; then
    if ! bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h 2>/dev/null; then
        echo "Warning: failed to generate vmlinux.h from BTF"
    fi
else
    echo "Warning: /sys/kernel/btf/vmlinux not found; compilation may fail"
fi

cat > test_bpf.c <<'EOF'
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

char LICENSE[] SEC("license") = "GPL";

SEC("tracepoint/syscalls/sys_enter_openat")
int trace_openat(void *ctx)
{
        bpf_printk("openat called");
        return 0;
}
EOF

COMPILE_CMD=(
    clang -O2 -g -target bpf
    -D__TARGET_ARCH_x86
    -I.
    -I/usr/include/bpf
    -c test_bpf.c -o test_bpf.o
)

echo "Compile command: ${COMPILE_CMD[*]}"

if "${COMPILE_CMD[@]}" 2>&1; then
        echo "✓ Successfully compiled test BPF program"
    
        # Try to load it
        if sudo bpftool prog load test_bpf.o /sys/fs/bpf/test_prog type tracepoint 2>&1; then
                echo "✓ Successfully loaded test BPF program"
                sudo rm -f /sys/fs/bpf/test_prog
        else
                echo "⚠ Compiled BPF program but couldn't load it (may need additional kernel features)"
        fi
else
        echo "✗ Failed to compile test BPF program"
        echo "(Using vmlinux.h to reduce kernel header noise; see output above.)"
fi

cd - >/dev/null
rm -rf "$TEST_BPF_DIR"

echo ""
echo "=========================================="
echo "Environment Ready!"
echo "=========================================="
echo ""
echo "Workspace directory: $TARGET_DIR"
echo ""
echo "Next steps:"
echo "  1. cd $TARGET_DIR"
echo "  2. mkdir build && cd build"
echo "  3. cmake .."
echo "  4. make"
echo ""
echo "Useful commands:"
echo "  - bpftool prog list              # List loaded BPF programs"
echo "  - bpftool map list               # List BPF maps"
echo "  - cat /sys/kernel/debug/tracing/trace_pipe  # View BPF printk output"
echo "  - sudo -i                        # Get root shell for BPF operations"
echo ""
} | tee "$LOG_FILE"
echo "Logs saved to: $LOG_FILE"
