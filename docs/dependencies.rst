============
Dependencies
============

DataCrumbs has several system and library dependencies that must be satisfied before building and running the tool.

System Requirements
===================

Operating System
----------------

DataCrumbs requires a Linux-based operating system with eBPF support. Tested distributions include:

* CentOS 8+
* Rocky Linux 8+
* Ubuntu 20.04+
* Debian 11+

Kernel Requirements
-------------------

**Recommended: Linux Kernel 5.8 or later**

DataCrumbs works best with modern kernel versions that have full eBPF feature support:

* **Linux 5.8+**: Full eBPF features including bounded loops, advanced helpers, and BPF ring buffers (recommended)
* **Linux 5.1+**: Supported with most features
* **Linux 4.18+**: Supported with compatibility layers (reduced functionality)

For kernels older than 5.8, DataCrumbs can use compatibility mechanisms, but some features may be limited.

To check your kernel version:

.. code-block:: bash

    uname -r

Kernel Configuration
--------------------

The kernel must be built with eBPF support enabled. Required kernel configuration options:

.. code-block:: text

    CONFIG_BPF=y
    CONFIG_BPF_SYSCALL=y
    CONFIG_BPF_JIT=y
    CONFIG_HAVE_EBPF_JIT=y
    CONFIG_BPF_EVENTS=y
    CONFIG_DEBUG_INFO_BTF=y

Most modern distributions enable these by default. To verify:

.. code-block:: bash

    # Check if BTF is available
    ls -la /sys/kernel/btf/vmlinux

    # Check BPF support
    zgrep BPF /proc/config.gz

Build Dependencies
==================

Compiler Requirements
---------------------

**GCC 11.2.0 or later** (recommended)

.. code-block:: bash

    # Check GCC version
    gcc --version

    # Set compiler for CMake
    export CC=$(which gcc)
    export CXX=$(which g++)

**Clang/LLVM** (for eBPF compilation)

Clang is required to compile eBPF programs. Version 10+ recommended.

.. code-block:: bash

    # Check Clang version
    clang --version

Build Tools
-----------

Required build tools:

* **CMake 3.5+**: Build system generator
* **Make**: Build automation
* **Git**: Version control (for obtaining source)
* **pkg-config**: Dependency detection

.. code-block:: bash

    # On CentOS/Rocky Linux
    sudo dnf install cmake make git pkg-config

    # On Ubuntu/Debian
    sudo apt-get install cmake build-essential git pkg-config

Core eBPF Dependencies
======================

libbpf (v1.5.0+)
----------------

**Required Version: 1.5.0 or later**

libbpf is the core library for loading and managing eBPF programs. DataCrumbs requires a recent version with support for:

* Modern BPF features (ring buffers, etc.)
* BTF support
* CO-RE (Compile Once - Run Everywhere)

Installation from Source
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    # Clone libbpf
    git clone https://github.com/libbpf/libbpf.git
    cd libbpf/src

    # Build and install
    make
    sudo make install
    sudo ldconfig

    # Verify installation
    pkg-config --modversion libbpf

Installation from Package Manager
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    # CentOS/Rocky Linux
    sudo dnf install libbpf-devel

    # Ubuntu 22.04+
    sudo apt-get install libbpf-dev

.. note::
   Package manager versions may be older than 1.5.0. Building from source is recommended for the latest features.

bpftool (v7.5.0+)
-----------------

**Required Version: 7.5.0 or later**

bpftool is used during the build process to generate eBPF-related headers and skeleton files.

Installation from Source
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    # Install dependencies
    sudo dnf install libelf-devel libcap-devel

    # Clone Linux kernel tools
    git clone --depth 1 --branch v6.1 https://github.com/torvalds/linux.git
    cd linux/tools/bpf/bpftool

    # Build and install
    make
    sudo make install

    # Verify installation
    bpftool version

System Installation
^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    # CentOS/Rocky Linux
    sudo dnf install bpftool

    # Ubuntu
    sudo apt-get install linux-tools-common linux-tools-generic

.. note::
   The bpftool version often matches the kernel version. Ensure you have a recent version installed.

Additional Library Dependencies
================================

yaml-cpp
--------

**Purpose**: Configuration file parsing (YAML format)

DataCrumbs uses YAML for configuration files that define:

* Host-specific settings
* Probe configurations
* Capture rules

Installation:

.. code-block:: bash

    # From source
    git clone https://github.com/jbeder/yaml-cpp.git
    cd yaml-cpp
    mkdir build && cd build
    cmake -DYAML_BUILD_SHARED_LIBS=ON ..
    make
    sudo make install

    # From package manager
    # CentOS/Rocky Linux
    sudo dnf install yaml-cpp-devel

    # Ubuntu/Debian
    sudo apt-get install libyaml-cpp-dev

json-c
------

**Purpose**: JSON data handling for trace output and data files

DataCrumbs uses JSON for:

* Probe metadata storage
* Category definitions
* Internal data exchange

Installation:

.. code-block:: bash

    # From package manager
    # CentOS/Rocky Linux
    sudo dnf install json-c-devel

    # Ubuntu/Debian
    sudo apt-get install libjson-c-dev

libelf
------

**Purpose**: ELF (Executable and Linkable Format) file parsing

Required for:

* Discovering symbols in libraries
* Attaching probes to functions
* Reading binary metadata

Installation:

.. code-block:: bash

    # CentOS/Rocky Linux
    sudo dnf install elfutils-libelf-devel

    # Ubuntu/Debian
    sudo apt-get install libelf-dev

zlib
----

**Purpose**: Trace data compression

DataCrumbs can compress trace output to reduce storage requirements.

Installation:

.. code-block:: bash

    # CentOS/Rocky Linux
    sudo dnf install zlib-devel

    # Ubuntu/Debian
    sudo apt-get install zlib1g-dev

Optional Dependencies
=====================

patchelf
--------

**Purpose**: Binary patching for track/untrack functionality

Required only if you plan to use ``datacrumbs_track`` and ``datacrumbs_untrack`` scripts to permanently instrument binaries.

Installation:

.. code-block:: bash

    # CentOS/Rocky Linux
    sudo dnf install patchelf

    # Ubuntu/Debian
    sudo apt-get install patchelf

MPI Implementation
------------------

**Purpose**: MPI-IO tracing support

If tracing MPI applications, install an MPI implementation:

.. code-block:: bash

    # OpenMPI
    sudo dnf install openmpi-devel

    # Or MPICH
    sudo dnf install mpich-devel

    # Load MPI module (if using environment modules)
    module load openmpi

HDF5
----

**Purpose**: HDF5 I/O tracing support

For applications using HDF5:

.. code-block:: bash

    # CentOS/Rocky Linux
    sudo dnf install hdf5-devel

    # Ubuntu/Debian
    sudo apt-get install libhdf5-dev

Python Dependencies (for Analysis)
===================================

The analysis tools in ``analysis/`` directory require Python 3.8+ with the following packages:

.. code-block:: bash

    pip install -r analysis/requirements.txt

Key packages:

- ``pandas``: Data analysis
- ``matplotlib``: Visualization
- ``jupyter``: Interactive notebooks
- ``numpy``: Numerical operations

Kernel Headers
==============

DataCrumbs needs access to kernel headers for eBPF compilation.

Installation:

.. code-block:: bash

    # CentOS/Rocky Linux
    sudo dnf install kernel-devel kernel-headers

    # Ubuntu/Debian
    sudo apt-get install linux-headers-$(uname -r)

The kernel headers should be located at:

- ``/usr/src/kernels/<kernel-version>``
- ``/lib/modules/$(uname -r)/build``

You can specify a custom path using the ``DATACRUMBS_KERNEL_HEADERS_PATH`` CMake variable.

Verifying Dependencies
======================

Before building, verify all required dependencies are available:

.. code-block:: bash

    # Check kernel version
    uname -r

    # Check for BTF support
    ls /sys/kernel/btf/vmlinux

    # Check libbpf
    pkg-config --modversion libbpf

    # Check bpftool
    bpftool version

    # Check compiler
    gcc --version
    clang --version

Complete Dependency Installation
=================================

CentOS/Rocky Linux 8
--------------------

.. code-block:: bash

    # System tools
    sudo dnf install -y gcc gcc-c++ clang llvm make cmake git
    sudo dnf install -y kernel-devel kernel-headers

    # eBPF tools
    sudo dnf install -y libbpf-devel bpftool

    # Libraries
    sudo dnf install -y elfutils-libelf-devel yaml-cpp-devel json-c-devel zlib-devel

    # Optional tools
    sudo dnf install -y patchelf

    # MPI (optional)
    sudo dnf install -y openmpi-devel

    # HDF5 (optional)
    sudo dnf install -y hdf5-devel

Ubuntu 22.04
------------

.. code-block:: bash

    # System tools
    sudo apt-get update
    sudo apt-get install -y build-essential clang llvm cmake git
    sudo apt-get install -y linux-headers-$(uname -r)

    # eBPF tools
    sudo apt-get install -y libbpf-dev linux-tools-common linux-tools-generic

    # Libraries
    sudo apt-get install -y libelf-dev libyaml-cpp-dev libjson-c-dev zlib1g-dev

    # Optional tools
    sudo apt-get install -y patchelf

    # MPI (optional)
    sudo apt-get install -y libopenmpi-dev

    # HDF5 (optional)
    sudo apt-get install -y libhdf5-dev

Troubleshooting
===============

BTF Not Available
-----------------

If ``/sys/kernel/btf/vmlinux`` doesn't exist:

1. Upgrade to a newer kernel (5.8+ recommended)
2. Rebuild kernel with ``CONFIG_DEBUG_INFO_BTF=y``
3. Use ``DATACRUMBS_KERNEL_HEADERS_PATH`` to point to kernel sources

libbpf Version Too Old
----------------------

If package manager version is < 1.5.0:

.. code-block:: bash

    # Build from source
    git clone https://github.com/libbpf/libbpf.git
    cd libbpf/src
    git checkout v1.5.0
    make
    sudo make install
    sudo ldconfig

Permission Issues
-----------------

eBPF operations require elevated privileges:

* Run DataCrumbs server components with ``sudo``
* Ensure user is in appropriate groups (e.g., ``bpf`` group if available)
* Check ``/proc/sys/kernel/unprivileged_bpf_disabled`` setting
