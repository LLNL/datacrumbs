# QEMU Native eBPF Testing VM

This directory contains scripts for launching lightweight Linux VMs using QEMU for eBPF development and testing.

## Quick Start

### 1. Download an Ubuntu cloud image (one-time setup)

```bash
cd /usr/workspace/haridev/datacrumbs/infrastructure/qemu
./run-qemu-ebpf.sh --download
```

### 2. Launch the VM

```bash
./run-qemu-ebpf.sh
```

The script will:
- Automatically inject your SSH public key (`~/.ssh/id_rsa.pub`)
- Mount the datacrumbs repository at `/mnt/datacrumbs` in the VM (via 9p)

This will boot a VM with:
- 4 vCPUs
- 4 GiB RAM
- 20 GiB disk
- Serial console interface
- TCG acceleration (no KVM required)
- Your SSH key pre-configured
@@- Datacrumbs repository mounted at `/mnt/datacrumbs`

### 3. Connect via SSH (after boot completes ~1-2 minutes)

```bash
ssh ubuntu@localhost -p 2222
```

### 4. Provision the VM (copy repo, install deps, test eBPF)

From the host, run the provisioning script:

```bash
infrastructure/qemu/provision-ebpf.sh --port 2222
```

This will:
- Wait for SSH readiness
- Copy the `datacrumbs` repository into the VM at `/home/ubuntu/datacrumbs`
- Run `setup-ebpf-vm.sh` inside the VM to install dependencies and perform a simple eBPF test

### 5. Build and test datacrumbs

```bash
cd /mnt/datacrumbs
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

## Management Commands

### Stop the VM
```bash
./run-qemu-ebpf.sh stop
```

### Clean up VM resources (stops VM and removes disk/logs)
```bash
./run-qemu-ebpf.sh clean
```

### Check if VM is running
```bash
ps aux | grep qemu | grep datacrumbs-ebpf
# or
tail -f /usr/workspace/haridev/datacrumbs/infrastructure/qemu/vm/console.log
```

Default credentials:
- **User**: `ubuntu`
- **Auth**: SSH key (automatically injected from `~/.ssh/id_rsa.pub`)

## Usage Examples

### Custom SSH Key
```bash
# Use a different SSH key
./run-qemu-ebpf.sh --ssh-key ~/.ssh/my_custom_key.pub
```

### Custom Resources
```bash
# 8 vCPUs, 8 GiB RAM
./run-qemu-ebpf.sh --cpus 8 --memory 8192

# Larger disk (50 GiB)
./run-qemu-ebpf.sh --disk-size 50

# Different SSH port
./run-qemu-ebpf.sh --port 3333
```

### Mount Host Directory (virtiofs preferred, 9p fallback)
```bash
./run-qemu-ebpf.sh --workspace /home/user/datacrumbs
```

Then inside the VM:
```bash
sudo mkdir -p /mnt/workspace
sudo mount -t virtiofs workspace /mnt/workspace || \
  sudo mount -t 9p -o trans=virtio,version=9p2000.L workspace /mnt/workspace
```

If neither mount backend is supported, use `provision-ebpf.sh` to copy the repo.

### GUI Mode
```bash
./run-qemu-ebpf.sh --with-gui
```

### Snapshot Mode (changes not persisted)
```bash
./run-qemu-ebpf.sh --snapshot
```

### Provisioning options
```bash
# Custom port and destination path
infrastructure/qemu/provision-ebpf.sh --port 2200 --dest /home/ubuntu/work/datacrumbs
```

## Setting Up eBPF Environment

Once logged in to the VM:

```bash
# Update package list
sudo apt-get update

# Install BPF development tools
sudo apt-get install -y \
  build-essential clang llvm \
  libelf-dev libpcap-dev libbfd-dev binutils-dev \
  linux-headers-$(uname -r) \
  bpf-tools libbpf-dev \
  bpftrace bcc-tools

# Check BPF is enabled
cat /proc/sys/kernel/unprivileged_bpf_disabled

# Test basic eBPF program
clang --version
llvm-objdump --version
```

## Advanced Configuration

### Environment Variables

```bash
# Override default image location
export IMAGE_PATH=/custom/path/to/image.img

# Set VM directory
export QEMU_DIR=/custom/qemu/directory

# Override defaults
export QEMU_CPUS=8
export QEMU_MEMORY=8192
export QEMU_DISK=50
export QEMU_SSH_PORT=3333
```

### Manual QEMU Configuration

If you need more control, you can directly invoke `qemu-system-x86_64`:

```bash
qemu-system-x86_64 \
  -machine type=pc \
  -cpu host \
  -smp cores=4 \
  -m 4096 \
  -drive file=./vm/session-disk.qcow2,format=qcow2 \
  -net user,hostfwd=tcp:127.0.0.1:2222-:22 \
  -net nic,model=virtio \
  -nographic -serial mon:stdio \
  -disable-kvm
```

## Stopping the VM

### From VM Console
- Press `Ctrl+C` or type `shutdown` command

### From Host
```bash
pkill -f "qemu-system-x86_64.*datacrumbs-ebpf"
```

## Troubleshooting

### Image Download Fails
- Download manually: `wget https://cloud-images.ubuntu.com/releases/22.04/release/ubuntu-22.04-server-cloudimg-amd64.img`
- Place in `./vm/` directory with the expected filename

### SSH Connection Refused
- Give the VM 1-2 minutes to boot and initialize cloud-init
- Check console output for any errors: `tail -f /tmp/qemu-console.log` (if available)

### 9p Mount Fails
- Ensure kernel support: `zgrep CONFIG_9P_FS /proc/config.gz`
- Use `trans=virtio` transport (not `trans=fd`)
- Check permissions on host directory

### Out of Memory
- Increase `--memory` parameter
- Check host available memory: `free -h`

### Slow Performance
- TCG emulation is inherently slow; KVM would be faster but requires `/dev/kvm`
- Reduce `--cpus` if system is under stress
- Run on a machine with faster CPU

## Performance Notes

- **TCG Mode**: This script uses TCG (Tiny Code Generator) for software emulation, which is slower than KVM but doesn't require hardware virtualization privileges
- **Cloud Image**: Ubuntu cloud images are lightweight (~2.2 GiB compressed)
- **First Boot**: Cloud-init initialization may take 1-2 minutes on first boot

## Directory Structure

```
infrastructure/qemu/
├── run-qemu-ebpf.sh     # Main launch script
├── setup-ebpf-vm.sh     # VM setup script (run inside the VM)
├── provision-ebpf.sh    # Host-side script to copy repo and run setup in VM
├── README.md            # This file
└── vm/                  # VM images and working directory
    ├── ubuntu-22.04-server-cloudimg-amd64.img  # Base image
    ├── session-disk.qcow2                       # Active VM disk
    ├── cloud-init.iso                           # Cloud-init config (SSH keys)
    └── console.log                              # VM console output
```

## See Also

- Podman machine: `../podman/run-toss5-machine.sh`
- Podman container: `../podman/run-toss5-container.sh`
- Docker container: `../docker/`
