# TOSS 5 Podman Dev Container

The `run-toss5-container.sh` helper launches the official LLNL TOSS 5 base
image (`wci-repo.llnl.gov:4567/lc-docker-public/toss-official/toss5-x86_64`)
with all of the mounts and privileges we use for the Docker dev container.
It automatically mounts the current repository at `/opt/datacrumbs`, binds
`/repo` for installing custom RPMs, and installs the development dependencies
from `infrastructure/docker/Dockerfile.build` (including `yaml-cpp` and
`json-c`) via `dnf`.

## Quick start

```bash
cd /home/haridev/datacrumbs
./infrastructure/podman/run-toss5-container.sh
```

By default the script:

- Mounts your workspace at `/opt/datacrumbs`
- Mounts `/repo` (override with `--repo-dir` or `DATACRUMBS_REPO_DIR`)
- Mounts `/lib/modules`, `/sys/kernel/debug`, and `/sys/fs/bpf`
- Runs as `--privileged` and adds `CAP_BPF` and `CAP_PERFMON`
- Bootstraps the container with `dnf` installing compiler, clang/LLVM,
  toolchain, OpenMPI, bpftool/libbpf, and the `yaml-cpp`/`json-c` packages

Use `--no-bootstrap` if the image is already provisioned, `--skip-pull`
when you have already `podman load`ed a tarball (avoids hitting a registry),
`--use-host-repos` to copy your host `/etc/yum.repos.d` + rpm GPG keys into
the container so `dnf` uses the same internal mirrors, `--workspace` to
override what is mounted at `/opt/datacrumbs`, or `--` followed by custom
flags to pass raw options to `podman run` (e.g. `-- --pull=never`).

## bpftool smoke test

Inside the container you can compile and load a trivial eBPF kprobe to
validate the environment:

```bash
cd /opt/datacrumbs
./infrastructure/podman/run-bpf-test.sh
```

The script generates `vmlinux.h` from `/sys/kernel/btf/vmlinux`, compiles
`tracepoint_bpf.c` with clang, loads it via `bpftool prog load`, and attaches
to `kprobe/sys_getpid`. Watch the output with `cat /sys/kernel/debug/tracing/trace_pipe`.
Cleanup instructions are printed after the script runs.

## Running inside a Podman machine VM

If your host account cannot load eBPF programs (for example, on systems where
you do not have sudo), you can run the development environment inside a Podman
machine virtual machine. The helper script will create (or reuse) a VM with your
workspace mounted inside it and then invoke the standard container launcher from
within the VM so the privileged eBPF pieces stay isolated.

```bash
cd /home/haridev/datacrumbs
./infrastructure/podman/run-toss5-machine.sh
```

The defaults create a machine named `datacrumbs-ebpf` with 4 vCPUs, 8 GiB RAM,
and a 60 GiB disk. Use `--cpus`, `--memory`, or `--disk-size` to tune the VM, or
`--machine-name` to keep multiple environments. Pass `--shell` if you just want
an interactive shell inside the VM (e.g. to run tools directly instead of
launching the container) or use `--` to forward options to
`run-toss5-container.sh`.

On first run the script uses `podman machine init -v ${PWD}:${PWD}` so the VM
can access your local checkout at the same absolute path. If you created a
machine manually before, recreate it with a matching `--volume` mount to make
the workspace visible.

The helper checks for `qemu-system-x86_64` and `virtiofsd` on your host and, if
they are only available under `/usr/libexec`, automatically drops tiny wrapper
scripts in `.podman-shims/` so Podman can find them. Set `PODMAN_QEMU_FALLBACK`
or `PODMAN_VIRTIOFSD_FALLBACK` if your distro installs those helpers somewhere
else.

If `/dev/kvm` is missing or unreadable (common on shared HPC systems), the
script automatically sets `PODMAN_MACHINE_NO_KVM=1` so QEMU falls back to TCG
software emulation. Remove any previously created machine (`podman machine rm
datacrumbs-ebpf`) and rerun the helper if you created it before this detection
was added. Running without KVM is significantly slower but avoids the need for
host-level virtualization privileges.

## Equivalent manual command

If you want to launch the container manually (mirroring the docker dev
container flags), the base invocation looks like:

```bash
podman run -ti \
  --privileged \
  --cap-add=CAP_BPF \
  --cap-add=CAP_PERFMON \
  --hostname podman \
  -v "$(pwd):/opt/datacrumbs" -w /opt/datacrumbs \
  -v /repo:/repo \
  -v /lib/modules/:/lib/modules:ro \
  -v /sys/kernel/debug/:/sys/kernel/debug:rw \
  -v /sys/fs/bpf:/sys/fs/bpf \
  wci-repo.llnl.gov:4567/lc-docker-public/toss-official/toss5-x86_64 \
  /bin/bash
```

Run `dnf -y install epel-release` followed by the dependency list from
`docker/Dockerfile.build` (gcc, clang, llvm, patchelf, gcc-toolset-11,
cmake, jq, time, openmpi-devel, bpftool, libbpf, yaml-cpp, json-c, the
`"Development Tools"` group, etc.) to mirror the scripted setup. If you need
the LLNL-internal `file:///repo/...` mirrors inside the container, start it
with `--use-host-repos` so those `.repo` files and GPG keys are copied in
before bootstrap runs.
