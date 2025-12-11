#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: run-qemu-ebpf.sh [command] [options]

Commands:
  start (default)      Start the VM
  stop                 Stop the running VM
  clean                Clean up VM disk and resources

Start Options:
  --image PATH         Path to Linux image (default: ubuntu-22.04-server-cloudimg-amd64.img)
  --cpus N             Number of vCPUs (default: 4)
  --memory SIZE        Memory in MiB (default: 4096 = 4 GiB)
  --disk-size SIZE     Additional disk size in GiB (default: 20 GiB)
  --port PORT          SSH port on host (default: 2222)
  --ssh-key PATH       Path to SSH public key to inject (default: ~/.ssh/id_rsa.pub)
  --workspace PATH     Host path to mount via 9p (optional; requires kernel support)
  --with-gui           Enable QEMU GUI instead of serial console
  --snapshot           Run with snapshot mode (don't persist changes)
  --download           Download Ubuntu cloud image if not present
  -h, --help           Show this message

Examples:
  # Start VM (will auto-inject your SSH key)
  ./run-qemu-ebpf.sh

  # Start with specific SSH key
  ./run-qemu-ebpf.sh --ssh-key ~/.ssh/my_key.pub

  # Stop the running VM
  ./run-qemu-ebpf.sh stop

  # Clean up VM resources
  ./run-qemu-ebpf.sh clean

  # With 8 vCPUs and 8 GiB RAM
  ./run-qemu-ebpf.sh --cpus 8 --memory 8192

USAGE
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
QEMU_DIR="${QEMU_DIR:-${SCRIPT_DIR}/vm}"
PID_FILE="${QEMU_DIR}/qemu.pid"
SESSION_DISK="${QEMU_DIR}/session-disk.qcow2"
CONSOLE_LOG="${QEMU_DIR}/console.log"
CLOUD_INIT_ISO="${QEMU_DIR}/cloud-init.iso"

# Defaults
COMMAND="start"
IMAGE_PATH="${IMAGE_PATH:-${QEMU_DIR}/ubuntu-22.04-server-cloudimg-amd64.img}"
CPUS="${QEMU_CPUS:-4}"
MEMORY="${QEMU_MEMORY:-4096}"
DISK_SIZE="${QEMU_DISK:-532}"
SSH_PORT="${QEMU_SSH_PORT:-2222}"
SSH_KEY_PATH="${SSH_KEY_PATH:-$HOME/.ssh/id_rsa.pub}"
SSH_KEY_PUBLIC="$SSH_KEY_PATH"
if [[ "$SSH_KEY_PATH" == *.pub ]]; then
  SSH_KEY_PRIVATE="${SSH_KEY_PATH%.pub}"
else
  SSH_KEY_PRIVATE="$SSH_KEY_PATH"
  SSH_KEY_PUBLIC="${SSH_KEY_PATH}.pub"
fi
WORKSPACE_PATH="${WORKSPACE_PATH:-$REPO_ROOT}"  # Default to repo root
WITH_GUI=0
SNAPSHOT=0
DOWNLOAD=0

# Parse command
if [[ $# -gt 0 ]] && [[ "$1" =~ ^(start|stop|clean)$ ]]; then
  COMMAND="$1"
  shift
fi

# Parse arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --image)
      [[ $# -lt 2 ]] && { echo "Missing value for --image" >&2; exit 1; }
      IMAGE_PATH="$2"
      shift 2
      ;;
    --ssh-key)
      [[ $# -lt 2 ]] && { echo "Missing value for --ssh-key" >&2; exit 1; }
      SSH_KEY_PATH="$2"
      shift 2
      ;;
    --cpus)
      [[ $# -lt 2 ]] && { echo "Missing value for --cpus" >&2; exit 1; }
      CPUS="$2"
      shift 2
      ;;
    --memory)
      [[ $# -lt 2 ]] && { echo "Missing value for --memory" >&2; exit 1; }
      MEMORY="$2"
      shift 2
      ;;
    --disk-size)
      [[ $# -lt 2 ]] && { echo "Missing value for --disk-size" >&2; exit 1; }
      DISK_SIZE="$2"
      shift 2
      ;;
    --port)
      [[ $# -lt 2 ]] && { echo "Missing value for --port" >&2; exit 1; }
      SSH_PORT="$2"
      shift 2
      ;;
    --workspace)
      [[ $# -lt 2 ]] && { echo "Missing value for --workspace" >&2; exit 1; }
      WORKSPACE_PATH="$2"
      shift 2
      ;;
    --with-gui)
      WITH_GUI=1
      shift
      ;;
    --nographic)
      WITH_GUI=0
      shift
      ;;
    --snapshot)
      SNAPSHOT=1
      shift
      ;;
    --download)
      DOWNLOAD=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

# Handle stop command
if [[ "$COMMAND" == "stop" ]]; then
  echo "Stopping datacrumbs-ebpf VM..."
  if pgrep -f "qemu.*datacrumbs-ebpf" >/dev/null; then
    pkill -f "qemu.*datacrumbs-ebpf"
    echo "VM stopped."
    # Wait a moment for cleanup
    sleep 2
  else
    echo "No VM is currently running."
  fi
  exit 0
fi

# Handle clean command
if [[ "$COMMAND" == "clean" ]]; then
  echo "Cleaning up datacrumbs-ebpf VM resources..."
  
  # Stop VM if running
  if pgrep -f "qemu.*datacrumbs-ebpf" >/dev/null; then
    echo "Stopping running VM..."
    pkill -f "qemu.*datacrumbs-ebpf"
    sleep 2
  fi
  # Stop virtiofsd if running and remove socket
  if pgrep -f virtiofsd >/dev/null; then
    echo "Stopping virtiofsd..."
    pkill -f virtiofsd || true
    sleep 1
  fi
  [[ -f "${QEMU_DIR}/virtiofs.sock" ]] && rm -f "${QEMU_DIR}/virtiofs.sock" && echo "Removed: ${QEMU_DIR}/virtiofs.sock"
  
  # Clean up files
  [[ -f "$SESSION_DISK" ]] && rm -f "$SESSION_DISK" && echo "Removed: $SESSION_DISK"
  [[ -f "$CONSOLE_LOG" ]] && rm -f "$CONSOLE_LOG" && echo "Removed: $CONSOLE_LOG"
  [[ -f "$CLOUD_INIT_ISO" ]] && rm -f "$CLOUD_INIT_ISO" && echo "Removed: $CLOUD_INIT_ISO"
  [[ -f "$PID_FILE" ]] && rm -f "$PID_FILE" && echo "Removed: $PID_FILE"
  
  echo "Cleanup complete."
  exit 0
fi

# From here on, we're doing 'start' command

# Ensure SSH key exists (generate default id_rsa if missing)
if [[ -f "$SSH_KEY_PUBLIC" ]]; then
  :
else
  if [[ "$SSH_KEY_PUBLIC" == "$HOME/.ssh/id_rsa.pub" ]]; then
    echo "SSH key not found; generating default key at $HOME/.ssh/id_rsa"
    mkdir -p "$HOME/.ssh"
    ssh-keygen -t rsa -b 4096 -f "$HOME/.ssh/id_rsa" -N ""
  else
    echo "Error: SSH key not found at $SSH_KEY_PUBLIC (and private $SSH_KEY_PRIVATE)" >&2
    exit 1
  fi
fi

# Check for QEMU availability
QEMU_BIN=""
if command -v qemu-system-x86_64 >/dev/null 2>&1; then
  QEMU_BIN="qemu-system-x86_64"
elif command -v qemu-kvm >/dev/null 2>&1; then
  QEMU_BIN="qemu-kvm"
elif [[ -x /usr/libexec/qemu-kvm ]]; then
  QEMU_BIN="/usr/libexec/qemu-kvm"
else
  echo "Error: QEMU not found" >&2
  echo "" >&2
  echo "Please install QEMU:" >&2
  echo "  RHEL/CentOS/Fedora: sudo dnf install qemu-kvm" >&2
  echo "  Ubuntu/Debian:      sudo apt-get install qemu-system-x86" >&2
  echo "  macOS:              brew install qemu" >&2
  echo "" >&2
  exit 1
fi
echo "Using QEMU: $QEMU_BIN"

# Ensure QEMU directory exists
mkdir -p "$QEMU_DIR"

# Download image if requested
if [[ "$DOWNLOAD" -eq 1 ]]; then
  if [[ ! -f "$IMAGE_PATH" ]]; then
    echo "Downloading Ubuntu 22.04 cloud image..."
    IMAGE_URL="https://cloud-images.ubuntu.com/releases/22.04/release/ubuntu-22.04-server-cloudimg-amd64.img"
    wget -O "$IMAGE_PATH" "$IMAGE_URL"
    echo "Downloaded to $IMAGE_PATH"
  else
    echo "Image already exists at $IMAGE_PATH"
  fi
fi

# Check if image exists
if [[ ! -f "$IMAGE_PATH" ]]; then
  echo "Error: Image not found at $IMAGE_PATH" >&2
  echo "You can download it with: --download" >&2
  exit 1
fi

# Create a copy for this session (so we can use snapshot mode or modifications)
SESSION_DISK="${QEMU_DIR}/session-disk.qcow2"
if [[ -f "$SESSION_DISK" ]]; then
  rm -f "$SESSION_DISK"
fi

# Convert or copy the image to qcow2 for this session
echo "Preparing disk image..."
if [[ "$IMAGE_PATH" == *.qcow2 ]]; then
  echo "Copying QCOW2 image..."
  cp "$IMAGE_PATH" "$SESSION_DISK"
else
  # Convert raw or compressed image to qcow2
  echo "Converting image to QCOW2 format (this may take a moment)..."
  if ! qemu-img convert -O qcow2 "$IMAGE_PATH" "$SESSION_DISK"; then
    echo "Error: Failed to convert image" >&2
    exit 1
  fi
fi
echo "Disk image ready: $SESSION_DISK"

CURRENT_SIZE_BYTES=$(qemu-img info "$SESSION_DISK" 2>/dev/null | awk -F'[()]' '/virtual size/ {print $2}' | awk '{print $1}' || echo "")
TARGET_SIZE_BYTES=$((DISK_SIZE * 1024 * 1024 * 1024))
if [[ -n "$CURRENT_SIZE_BYTES" ]] && [[ "$CURRENT_SIZE_BYTES" =~ ^[0-9]+$ ]] && (( CURRENT_SIZE_BYTES < TARGET_SIZE_BYTES )); then
  CURRENT_SIZE_GIB=$(( CURRENT_SIZE_BYTES / 1024 / 1024 / 1024 ))
  echo "Resizing disk from ${CURRENT_SIZE_GIB}G to ${DISK_SIZE}G..."
  if qemu-img resize "$SESSION_DISK" "${DISK_SIZE}G"; then
    echo "Disk resized successfully"
  else
    echo "Warning: Could not resize disk" >&2
  fi
else
  echo "Disk resize skipped (current: ${CURRENT_SIZE_BYTES:-unknown} bytes, target: ${TARGET_SIZE_BYTES} bytes)"
fi

# Create cloud-init ISO for SSH key injection
echo "Creating cloud-init configuration..."
CLOUD_INIT_DIR="${QEMU_DIR}/cloud-init-tmp"
rm -rf "$CLOUD_INIT_DIR"
mkdir -p "$CLOUD_INIT_DIR"

# Check if SSH key exists
if [[ -f "$SSH_KEY_PUBLIC" ]]; then
  SSH_KEY_CONTENT=$(cat "$SSH_KEY_PUBLIC")
  echo "Using SSH key: $SSH_KEY_PUBLIC"
else
  echo "Warning: SSH key not found at $SSH_KEY_PUBLIC"
  echo "VM will boot but you won't be able to SSH without a key."
  SSH_KEY_CONTENT=""
fi

# Create meta-data
cat > "$CLOUD_INIT_DIR/meta-data" <<EOF
instance-id: datacrumbs-ebpf-$(date +%s)
local-hostname: datacrumbs-ebpf
EOF

# Create user-data with SSH key
cat > "$CLOUD_INIT_DIR/user-data" <<EOF
#cloud-config
users:
  - name: ubuntu
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash
    ssh_authorized_keys:
      - $SSH_KEY_CONTENT
ssh_pwauth: false
disable_root: true
package_update: false
package_upgrade: false
growpart:
  mode: auto
  devices: ['/dev/vda']
  ignore_growroot_disabled: false
fs_setup:
  - label: root
    filesystem: ext4
    device: /dev/vda1
    overwrite: false
runcmd:
  - mkdir -p /mnt/datacrumbs
  - mount -t 9p -o trans=virtio,version=9p2000.L workspace /mnt/datacrumbs || true
  - mount -t virtiofs workspace /mnt/datacrumbs || true
  - chown ubuntu:ubuntu /mnt/datacrumbs || true
EOF

# Create ISO
if command -v genisoimage >/dev/null 2>&1; then
  genisoimage -output "$CLOUD_INIT_ISO" -volid cidata -joliet -rock "$CLOUD_INIT_DIR/user-data" "$CLOUD_INIT_DIR/meta-data" >/dev/null 2>&1
elif command -v mkisofs >/dev/null 2>&1; then
  mkisofs -output "$CLOUD_INIT_ISO" -volid cidata -joliet -rock "$CLOUD_INIT_DIR/user-data" "$CLOUD_INIT_DIR/meta-data" >/dev/null 2>&1
else
  echo "Warning: genisoimage/mkisofs not found. Cloud-init ISO not created."
  echo "Install with: sudo dnf install genisoimage"
  CLOUD_INIT_ISO=""
fi

rm -rf "$CLOUD_INIT_DIR"

if [[ -n "$CLOUD_INIT_ISO" ]] && [[ -f "$CLOUD_INIT_ISO" ]]; then
  echo "Cloud-init ISO created: $CLOUD_INIT_ISO"
fi

# Build QEMU command
QEMU_ARGS=(
  -name "datacrumbs-ebpf"
  -machine type=pc
  -cpu qemu64
  -smp "${CPUS}"
  -m "${MEMORY}"
  -drive "file=${SESSION_DISK},format=qcow2,if=virtio"
  -netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${SSH_PORT}-:22"
  -device "virtio-net-pci,netdev=net0"
  -accel tcg
  -daemonize
)

# Add cloud-init ISO if it was created
if [[ -n "$CLOUD_INIT_ISO" ]] && [[ -f "$CLOUD_INIT_ISO" ]]; then
  QEMU_ARGS+=(-drive "file=${CLOUD_INIT_ISO},format=raw,if=virtio,media=cdrom")
fi

# Workspace mount (9p or virtiofs)
add_workspace_mount() {
  if [[ -z "$WORKSPACE_PATH" ]]; then
    return
  fi
  if [[ ! -d "$WORKSPACE_PATH" ]]; then
    echo "Warning: Workspace path '$WORKSPACE_PATH' does not exist; skipping mount." >&2
    return
  fi
  WORKSPACE_PATH="$(cd "$WORKSPACE_PATH" && pwd)"

  # Prefer virtiofs if available; wait for socket readiness
  if command -v virtiofsd >/dev/null 2>&1 || [[ -x /usr/libexec/virtiofsd ]]; then
    VIRTIOFSD_BIN=$(command -v virtiofsd || echo /usr/libexec/virtiofsd)
    VFS_SOCK="${QEMU_DIR}/virtiofs.sock"
    rm -f "$VFS_SOCK"
    echo "Starting virtiofsd for $WORKSPACE_PATH..."
    "$VIRTIOFSD_BIN" --socket-path="$VFS_SOCK" -o source="$WORKSPACE_PATH" -o cache=always -o sandbox=none >/dev/null 2>&1 &
    VFS_PID=$!
    # Wait up to 5s for socket
    for i in 1 2 3 4 5; do
      if [[ -S "$VFS_SOCK" ]]; then
        break
      fi
      sleep 1
    done
    if [[ -S "$VFS_SOCK" ]]; then
      QEMU_ARGS+=(-chardev "socket,id=char0,path=${VFS_SOCK}")
      QEMU_ARGS+=(-device "vhost-user-fs-pci,chardev=char0,tag=workspace")
      echo "Note: Inside VM, mount with: mount -t virtiofs workspace /mnt/datacrumbs"
      return
    else
      echo "Warning: virtiofsd socket not ready; falling back."
      # Kill failed virtiofsd if still running
      [[ -n "${VFS_PID:-}" ]] && kill "${VFS_PID}" >/dev/null 2>&1 || true
    fi
  fi

  # Fallback to 9p if supported by this QEMU build
  if "$QEMU_BIN" -device help 2>/dev/null | grep -q "virtio-9p-pci"; then
    QEMU_ARGS+=(-fsdev "local,security_model=passthrough,id=fsdev0,path=${WORKSPACE_PATH}")
    QEMU_ARGS+=(-device "virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=workspace")
    echo "Note: Inside VM, mount with: mount -t 9p -o trans=virtio workspace /mnt/datacrumbs"
  else
    echo "Warning: QEMU build does not support 9p (virtio-9p-pci)."
    echo "Warning: Neither virtiofs nor 9p available; skipping workspace mount."
  fi
}

# Add GUI or serial console
# Note: When daemonizing, we can't use stdio, so log to a file instead
CONSOLE_LOG="${QEMU_DIR}/console.log"
if [[ "$WITH_GUI" -eq 1 ]]; then
  QEMU_ARGS+=(-display gtk)
else
  QEMU_ARGS+=(
    -display none
    -serial "file:${CONSOLE_LOG}"
  )
  echo "Console output will be logged to: $CONSOLE_LOG"
fi

# Add snapshot mode
if [[ "$SNAPSHOT" -eq 1 ]]; then
  QEMU_ARGS+=(-snapshot)
fi

# Print startup info
MEMORY_GIB=$((MEMORY / 1024))
echo "================================================================================"
echo "Starting QEMU VM for eBPF Testing"
echo "================================================================================"
echo "CPU Cores:     $CPUS"
echo "Memory:        ${MEMORY} MiB (${MEMORY_GIB} GiB)"
echo "Disk:          ${DISK_SIZE} GiB"
echo "Image:         $IMAGE_PATH"
echo "SSH Port:      $SSH_PORT"
if [[ "$WITH_GUI" -eq 1 ]]; then
  echo "Display:       GUI"
else
  echo "Display:       Serial Console"
fi
if [[ "$SNAPSHOT" -eq 1 ]]; then
  echo "Snapshot Mode: Enabled"
else
  echo "Snapshot Mode: Disabled"
fi
echo ""
echo "Default credentials for Ubuntu cloud image:"
echo "  User: ubuntu"
echo "  SSH:  ssh ubuntu@localhost -p $SSH_PORT"
echo ""
echo "Boot-time Setup:"
echo "  The VM will perform cloud-init initialization on first boot."
echo "  This may take 1-2 minutes. Check the console for progress."
echo ""
echo "To stop the VM:"
echo "  - In console: Ctrl+C or type 'shutdown' command"
echo "  - From host: pkill -f qemu-system-x86_64"
echo ""
echo "eBPF Testing:"
echo "  1. Log in via SSH or console"
echo "  2. Install dependencies: sudo apt-get update && sudo apt-get install -y \\"
echo "       build-essential clang llvm libelf-dev libpcap-dev libbfd-dev \\"
echo "       binutils-dev linux-headers-\$(uname -r) bpf-tools libbpf-dev"
echo "  3. Test with: cat /proc/sys/kernel/unprivileged_bpf_disabled"
echo ""
echo "VM disk:  $SESSION_DISK"
echo "QEMU dir: $QEMU_DIR"
echo "================================================================================"

echo ""
echo "Launching QEMU with TCG (software emulation - may be slow)..."
echo ""

# Try to add workspace mount now (appends to QEMU_ARGS)
add_workspace_mount

# Run QEMU (don't use exec so we can see errors)
"$QEMU_BIN" "${QEMU_ARGS[@]}"

# Wait for VM to boot and become SSH-ready
echo ""
echo "Waiting for VM to boot and SSH to be ready..."
BOOT_TIMEOUT=300  # 5 minutes
BOOT_INTERVAL=5
ELAPSED=0

while [[ $ELAPSED -lt $BOOT_TIMEOUT ]]; do
  if ssh -o BatchMode=yes -o StrictHostKeyChecking=no -o ConnectTimeout=2 \
    -i "${SSH_KEY_PRIVATE}" -p "$SSH_PORT" ubuntu@localhost true 2>/dev/null; then
    printf "\r✓ VM is ready!%*s\n" 20 ""
    echo ""
    echo "You can now SSH in with:"
    echo "  ssh ubuntu@localhost -p $SSH_PORT"
    echo ""
    exit 0
  fi
  printf "\r  Waiting... (%ds/%ds)" "$ELAPSED" "$BOOT_TIMEOUT"
  sleep $BOOT_INTERVAL
  ELAPSED=$((ELAPSED + BOOT_INTERVAL))
done

echo ""
echo "⚠ VM did not become SSH-ready within ${BOOT_TIMEOUT}s"
echo "  Check the console: tail -f $CONSOLE_LOG"
exit 1
