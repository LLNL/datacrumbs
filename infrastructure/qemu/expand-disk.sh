#!/usr/bin/env bash
# Expand the root filesystem to use all available disk space
# Run this inside the VM if the filesystem is not auto-expanded during boot

set -euo pipefail

echo "Expanding root filesystem..."

echo "Installing growpart and filesystem tools (cloud-guest-utils, e2fsprogs, parted)..."
sudo apt-get update -qq
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y -qq cloud-guest-utils e2fsprogs parted >/dev/null

BEFORE=$(df -h / | tail -1)
echo "Before: $BEFORE"

# Identify root device (usually /dev/vda1)
ROOT_DEV=$(df / | tail -1 | awk '{print $1}')
echo "Root device: $ROOT_DEV"

# Identify disk (remove partition number)
DISK=$(echo "$ROOT_DEV" | sed 's/[0-9]*$//')
echo "Disk: $DISK"

# Expand partition
echo "Expanding partition..."
if sudo growpart "$DISK" 1; then
	echo "growpart succeeded"
else
	echo "growpart failed, trying parted..."
	sudo parted -s "$DISK" resizepart 1 100% || echo "Warning: Could not expand partition"
fi

# Resize filesystem (ext4 assumed)
echo "Resizing filesystem..."
if sudo resize2fs "$ROOT_DEV"; then
	echo "resize2fs succeeded"
else
	echo "resize2fs failed; attempting e2fsck then retry"
	sudo e2fsck -f "$ROOT_DEV" || true
	sudo resize2fs "$ROOT_DEV" || echo "Warning: Could not resize filesystem"
fi

# Verify
AFTER=$(df -h / | tail -1)
echo "After:  $AFTER"
echo "✓ Expansion complete"
