#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: run-toss5-container.sh [options]

Launches the LLNL TOSS 5 development container with Podman, preps dependencies,
and mounts the current workspace plus /repo for package installs.

Options:
  --workspace PATH     Host path to mount at /opt/datacrumbs (default: repo root)
  --repo-dir PATH      Host path to mount into /repo inside the container (default: /repo)
  --name NAME          Podman container name (default: datacrumbs-toss5-dev)
  --image IMAGE        Container image to run (default: TOSS5 base image)
  --no-bootstrap       Skip the dnf installation/bootstrap commands
  --skip-pull          Do not call 'podman pull' before running
  --use-host-repos     Copy host /etc/yum.repos.d and rpm-gpg keys into the container
  --host-repo-path     Override host repo directory (default: /etc/yum.repos.d)
  --host-pki-path      Override host rpm-gpg directory (default: /etc/pki/rpm-gpg)
  -h, --help           Show this message

Use -- to pass additional args directly to podman run.
USAGE
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

PODMAN_BIN="${PODMAN_BIN:-podman}"
PODMAN_IMAGE="${PODMAN_IMAGE:-wci-repo.llnl.gov:4567/lc-docker-public/toss-official/toss5-x86_64}"
CONTAINER_NAME="${CONTAINER_NAME:-datacrumbs-toss5-dev}"
WORKSPACE="${DATACRUMBS_WORKSPACE:-$REPO_ROOT}"
REPO_DIR="${DATACRUMBS_REPO_DIR:-/repo}"
HOST_REPO_SRC="${DATACRUMBS_HOST_REPO_SRC:-/etc/yum.repos.d}"
HOST_PKI_SRC="${DATACRUMBS_HOST_PKI_SRC:-/etc/pki/rpm-gpg}"
SKIP_BOOTSTRAP=0
SKIP_PULL=0
USE_HOST_REPOS="${DATACRUMBS_USE_HOST_REPOS:-0}"
EXTRA_ARGS=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --workspace)
      [[ $# -lt 2 ]] && { echo "Missing value for --workspace" >&2; exit 1; }
      WORKSPACE="$2"
      shift 2
      ;;
    --repo-dir)
      [[ $# -lt 2 ]] && { echo "Missing value for --repo-dir" >&2; exit 1; }
      REPO_DIR="$2"
      shift 2
      ;;
    --name)
      [[ $# -lt 2 ]] && { echo "Missing value for --name" >&2; exit 1; }
      CONTAINER_NAME="$2"
      shift 2
      ;;
    --image)
      [[ $# -lt 2 ]] && { echo "Missing value for --image" >&2; exit 1; }
      PODMAN_IMAGE="$2"
      shift 2
      ;;
    --no-bootstrap)
      SKIP_BOOTSTRAP=1
      shift
      ;;
    --skip-pull)
      SKIP_PULL=1
      shift
      ;;
    --use-host-repos)
      USE_HOST_REPOS=1
      shift
      ;;
    --host-repo-path)
      [[ $# -lt 2 ]] && { echo "Missing value for --host-repo-path" >&2; exit 1; }
      HOST_REPO_SRC="$2"
      shift 2
      ;;
    --host-pki-path)
      [[ $# -lt 2 ]] && { echo "Missing value for --host-pki-path" >&2; exit 1; }
      HOST_PKI_SRC="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      EXTRA_ARGS+=("$@")
      break
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if ! command -v "$PODMAN_BIN" >/dev/null 2>&1; then
  echo "Podman is required but was not found in PATH." >&2
  exit 1
fi

if [[ ! -d "$WORKSPACE" ]]; then
  echo "Workspace path '$WORKSPACE' does not exist." >&2
  exit 1
fi
WORKSPACE="$(cd "$WORKSPACE" && pwd)"

if [[ ! -d "$REPO_DIR" ]]; then
  if ! mkdir -p "$REPO_DIR" 2>/dev/null; then
    echo "Repository directory '$REPO_DIR' does not exist and could not be created." >&2
    echo "Create it manually or point --repo-dir/DATACRUMBS_REPO_DIR to a writable path." >&2
    exit 1
  fi
fi
REPO_DIR="$(cd "$REPO_DIR" && pwd)"

HOST_REPO_DEST="/host/etc/yum.repos.d"
HOST_PKI_DEST="/host/etc/pki/rpm-gpg"
HOST_PKI_AVAILABLE=0

if [[ "$USE_HOST_REPOS" -eq 1 ]]; then
  if [[ ! -d "$HOST_REPO_SRC" ]]; then
    echo "Host repo directory '$HOST_REPO_SRC' does not exist." >&2
    exit 1
  fi
  HOST_REPO_SRC="$(cd "$HOST_REPO_SRC" && pwd)"

  if [[ -d "$HOST_PKI_SRC" ]]; then
    HOST_PKI_SRC="$(cd "$HOST_PKI_SRC" && pwd)"
    HOST_PKI_AVAILABLE=1
  else
    echo "Warning: host rpm-gpg directory '$HOST_PKI_SRC' not found; skipping key sync." >&2
    HOST_PKI_AVAILABLE=0
  fi
fi

DNF_PACKAGES=(
  gcc clang llvm patchelf sudo elfutils-libelf-devel kernel-devel kernel-headers
  make iproute iputils git vim-enhanced which python3-pip cmake llvm-devel clang-devel
  gcc-toolset-12 jq time openmpi-4.1-gnu bpftool libbpf libbpf-devel
  yaml-cpp yaml-cpp-devel json-c json-c-devel
)

PACKAGE_LIST="${DNF_PACKAGES[*]}"

if [[ "$SKIP_BOOTSTRAP" -eq 0 ]]; then
  BOOTSTRAP_EXTRA=""
  if [[ "$USE_HOST_REPOS" -eq 1 ]]; then
    BOOTSTRAP_EXTRA=$(cat <<'EOS'
if [ -d /host/etc/yum.repos.d ]; then
  echo "Using host yum repo definitions from /host/etc/yum.repos.d"
  rm -f /etc/yum.repos.d/*.repo || true
  cp -a --no-preserve=ownership /host/etc/yum.repos.d/* /etc/yum.repos.d/
fi
if [ -d /host/etc/pki/rpm-gpg ]; then
  echo "Copying host rpm-gpg keys from /host/etc/pki/rpm-gpg"
  cp -a --no-preserve=ownership /host/etc/pki/rpm-gpg/* /etc/pki/rpm-gpg/ || true
fi
EOS
)
  fi

  read -r -d '' BOOTSTRAP_CMD <<EOF || true
set -euo pipefail
$BOOTSTRAP_EXTRA
if ! command -v dnf >/dev/null 2>&1; then
  echo "dnf is required inside the container but was not found." >&2
  exec /bin/bash
fi
dnf -y makecache
dnf -y update
dnf -y install ${PACKAGE_LIST}
dnf clean all
exec /bin/bash
EOF
  CONTAINER_ENTRY=(/bin/bash -lc "$BOOTSTRAP_CMD")
else
  CONTAINER_ENTRY=(/bin/bash)
fi

PODMAN_ARGS=(
  run --rm -ti
  --name "$CONTAINER_NAME"
  --privileged
  --cap-add=CAP_BPF
  --cap-add=CAP_PERFMON
  --hostname podman
  -v "$WORKSPACE:/opt/datacrumbs:Z"
  -w /opt/datacrumbs
  -v /lib/modules/:/lib/modules:ro
  -v /sys/kernel/debug/:/sys/kernel/debug:rw
  -v /sys/fs/bpf:/sys/fs/bpf
  -v "$REPO_DIR:/repo"
)

if [[ ${#EXTRA_ARGS[@]} -gt 0 ]]; then
  PODMAN_ARGS+=("${EXTRA_ARGS[@]}")
fi

if [[ "$USE_HOST_REPOS" -eq 1 ]]; then
  PODMAN_ARGS+=(-v "$HOST_REPO_SRC:${HOST_REPO_DEST}:ro")
  if [[ "$HOST_PKI_AVAILABLE" -eq 1 ]]; then
    PODMAN_ARGS+=(-v "$HOST_PKI_SRC:${HOST_PKI_DEST}:ro")
  fi
fi

if [[ "$SKIP_PULL" -eq 0 ]]; then
  echo "Pulling latest image ${PODMAN_IMAGE}..."
  "$PODMAN_BIN" pull "$PODMAN_IMAGE" >/dev/null
else
  echo "Skipping podman pull for ${PODMAN_IMAGE}"
fi

echo "Starting TOSS5 container with workspace ${WORKSPACE} mounted at /opt/datacrumbs"
set -x
"$PODMAN_BIN" "${PODMAN_ARGS[@]}" "$PODMAN_IMAGE" "${CONTAINER_ENTRY[@]}"
