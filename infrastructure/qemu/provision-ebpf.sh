#!/usr/bin/env bash
set -euo pipefail

# Host-side provisioning: copy repo into VM, install deps, run eBPF test

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Defaults
HOST="localhost"
PORT="${QEMU_SSH_PORT:-2222}"
USER="ubuntu"
IDENTITY="${SSH_IDENTITY:-$HOME/.ssh/id_rsa}"
DEST="/home/ubuntu/datacrumbs"
INIT=0

usage() {
  cat <<USAGE
Usage: provision-ebpf.sh [options]

Options:
  --host HOST         SSH host (default: localhost)
  --port PORT         SSH port (default: 2222)
  --user USER         SSH user (default: ubuntu)
  --identity PATH     SSH private key (default: ~/.ssh/id_rsa)
  --dest PATH         Destination path in VM (default: /home/ubuntu/datacrumbs)
  --init              First-time init: expand filesystem before copying
  -h, --help          Show this message

Steps performed:
  --init provided:
    1) Waits for SSH to be ready
    2) Expands VM filesystem to use full disk
    3) Copies the datacrumbs repo into VM at --dest
    4) Runs setup script inside VM to install deps and test eBPF
  (default without --init):
    1) Waits for SSH to be ready
    2) Copies the datacrumbs repo into VM at --dest
    3) Runs setup script inside VM to install deps and test eBPF
USAGE
}

# Parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)
      [[ $# -lt 2 ]] && { echo "Missing value for --host" >&2; exit 1; }
      HOST="$2"; shift 2 ;;
    --port)
      [[ $# -lt 2 ]] && { echo "Missing value for --port" >&2; exit 1; }
      PORT="$2"; shift 2 ;;
    --user)
      [[ $# -lt 2 ]] && { echo "Missing value for --user" >&2; exit 1; }
      USER="$2"; shift 2 ;;
    --identity)
      [[ $# -lt 2 ]] && { echo "Missing value for --identity" >&2; exit 1; }
      IDENTITY="$2"; shift 2 ;;
    --dest)
      [[ $# -lt 2 ]] && { echo "Missing value for --dest" >&2; exit 1; }
      DEST="$2"; shift 2 ;;
    --init)
      INIT=1; shift ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "Unknown option: $1" >&2; usage; exit 1 ;;
  esac
done

echo "Repo root: $REPO_ROOT"
echo "Target VM: $USER@$HOST:$PORT"
echo "Destination: $DEST"

# Check identity
SSH_ID_OPT=()
if [[ -f "$IDENTITY" ]]; then
  SSH_ID_OPT=(-i "$IDENTITY")
else
  echo "Warning: SSH identity not found at $IDENTITY; relying on default agent/keys."
fi

# Wait for SSH readiness (up to 90s)
echo "Waiting for SSH to be ready..."
READY=0
for i in {1..45}; do
  if ssh -o BatchMode=yes -o StrictHostKeyChecking=no -o ConnectTimeout=2 "${SSH_ID_OPT[@]}" -p "$PORT" "$USER@$HOST" true 2>/dev/null; then
    READY=1; break
  fi
  sleep 2
done
if [[ "$READY" -ne 1 ]]; then
  echo "Error: SSH not ready on $HOST:$PORT after waiting." >&2
  exit 1
fi
echo "✓ SSH is ready"

if [[ "$INIT" -eq 1 ]]; then
  echo "Expanding filesystem in VM (init mode)..."
  EXPAND_SCRIPT="$SCRIPT_DIR/expand-disk.sh"
  if [[ -f "$EXPAND_SCRIPT" ]]; then
    scp "${SSH_ID_OPT[@]}" -P "$PORT" "$EXPAND_SCRIPT" "$USER@$HOST:/tmp/"
    ssh -o StrictHostKeyChecking=no "${SSH_ID_OPT[@]}" -p "$PORT" "$USER@$HOST" "bash -lc 'bash /tmp/expand-disk.sh && rm /tmp/expand-disk.sh'" || echo "Warning: Could not expand disk"
  else
    echo "Warning: expand-disk.sh not found; skipping expansion"
  fi
else
  echo "Init flag not set; skipping filesystem expansion."
fi

# Create destination directory
ssh -o StrictHostKeyChecking=no "${SSH_ID_OPT[@]}" -p "$PORT" "$USER@$HOST" "mkdir -p '$DEST'"

# Copy repo (prefer rsync with excludes from .gitignore; fallback to scp)
if command -v rsync >/dev/null 2>&1; then
  echo "Copying repository via rsync..."
  # Use .gitignore to filter excludes
  rsync -az --delete --filter=':- .gitignore' -e "ssh -p $PORT ${SSH_ID_OPT[*]}" "$REPO_ROOT/" "$USER@$HOST:$DEST/"
else
  echo "rsync not found; using scp (no excludes)."
  scp -r "${SSH_ID_OPT[@]}" -P "$PORT" "$REPO_ROOT" "$USER@$HOST:$(dirname "$DEST")"
fi
echo "✓ Repository copied to VM"

# Run setup script inside VM
REMOTE_SETUP="$DEST/infrastructure/qemu/setup-ebpf-vm.sh"
LOCAL_LOG="$SCRIPT_DIR/provision.log"
echo "Running setup inside VM (logging to $LOCAL_LOG)..."
ssh -t -o StrictHostKeyChecking=no "${SSH_ID_OPT[@]}" -p "$PORT" "$USER@$HOST" \
  "bash -lc 'chmod +x \"$REMOTE_SETUP\" && \"$REMOTE_SETUP\" --workspace \"$DEST\"'" | tee "$LOCAL_LOG"

echo "✓ Provisioning complete"
echo "Host log: $LOCAL_LOG"
echo "VM log: /var/log/datacrumbs-setup.log"
