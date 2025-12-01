=================
Building DataCrumbs
=================

This comprehensive guide covers building DataCrumbs from source in various environments, including standard builds, HPC systems, Docker containers, and the Tuolumne supercomputer. It includes complete instructions for installing dependencies with custom prefix paths.

Prerequisites
=============

Before building, ensure all dependencies are installed as described in the :doc:`dependencies` section.

Quick verification:

.. code-block:: bash

    # Verify kernel version (5.8+ recommended)
    uname -r

    # Verify BTF support
    ls /sys/kernel/btf/vmlinux

    # Verify libbpf
    pkg-config --modversion libbpf

    # Verify bpftool
    bpftool version

Core Requirements
-----------------

**Build Tools:**

* Git
* CMake (3.14+)
* GCC or Clang
* Python (for Sphinx documentation)
* Linux kernel 5.8+ with BTF support

Obtaining the Source
====================

Clone the DataCrumbs repository:

.. code-block:: bash

    git clone https://github.com/LLNL/datacrumbs.git
    cd datacrumbs
    export DATACRUMBS_DIR=$(realpath .)

Standard Build Process
======================

Basic Build
-----------

The simplest build uses default configuration:

.. code-block:: bash

    # Create build directory
    mkdir build
    cd build

    # Configure with CMake
    cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..

    # Build
    make -j$(nproc)

    # Install
    make install

This will:

1. Discover available probes in system libraries
2. Generate custom eBPF programs
3. Compile eBPF and C++ code
4. Install binaries, libraries, and scripts

Custom Prefix Installation
===========================

This section describes how to build and install all dependencies and DataCrumbs under a custom prefix directory. This is essential for non-root installations and HPC environments.

Set Installation Prefix
-----------------------

All dependencies and DataCrumbs will be installed under a custom prefix:

.. code-block:: bash

    export PREFIX=/your/custom/prefix
    # For example: export PREFIX=$HOME/datacrumbs-install
    # Or on HPC: export PREFIX=/usr/workspace/$USER/datacrumbs-install

Build and Install Dependencies
-------------------------------

The following dependencies are required for building DataCrumbs:

**Runtime Dependencies:**

* bpftool (v7.5.0) and libbpf (v1.5.0)
* json-c
* yaml-cpp
* llvm
- (Optional) bpftime for user-space (experimental)

**1. Build bpftool (v7.5.0) and libbpf (v1.5.0)**

.. code-block:: bash

    git clone https://github.com/libbpf/bpftool.git
    pushd bpftool
    git checkout tags/v7.5.0 -b v7.5.0
    git submodule update --init --recursive

    # Build libbpf
    pushd libbpf
    git checkout tags/v1.5.0 -b v1.5.0
    cd src
    DESTDIR=$PREFIX make install -j
    popd

    # Build bpftool
    cd src
    DESTDIR=$PREFIX make install -j
    popd

.. important::
   **Understanding Prefix-Style Installation Structure**

   When building with ``DESTDIR=$PREFIX``, the build system installs files into nested directories
   under ``$PREFIX`` (e.g., ``$PREFIX/usr/local/lib`` instead of ``$PREFIX/lib``). You must move
   these files to the correct locations within your ``$PREFIX`` directory for DataCrumbs to find them.

**2. Move Files to Correct Prefix Structure**

After building, use ``find`` to locate and move files to the correct locations:

.. code-block:: bash

    pushd $PREFIX

    # Find and move bpf.h and libbpf
    bpf_header=$(find . -name bpf.h | head -n 1)
    bpf_header=$(readlink -f $bpf_header)
    bpf_install_dir=$(dirname $(dirname $(dirname $bpf_header)))
    if [[ "$bpf_install_dir" != "$PREFIX" ]]; then
        mv $bpf_install_dir/include $PREFIX
        mv $bpf_install_dir/lib* $PREFIX
    fi

    # Find and move bpftool
    bpftool=$(find . -name bpftool | head -n 1)
    bpftool=$(readlink -f $bpftool)
    bpftool_install_dir=$(dirname $(dirname $bpftool))
    if [[ "$bpftool_install_dir" != "$PREFIX" ]]; then
        mv $bpftool_install_dir/* $PREFIX
    fi

    popd

**3. Verify Installation**

Check that files are in the correct locations:

.. code-block:: bash

    echo "Checking installed files under \$PREFIX:"
    echo "bpf.h:"
    find $PREFIX -name bpf.h

    echo "libbpf.so:"
    find $PREFIX -name libbpf.so

    echo "libbpf.pc:"
    find $PREFIX -name libbpf.pc

    echo "bpftool:"
    find $PREFIX -name bpftool

Expected output (with PREFIX=/home/user/install):

.. code-block:: text

    bpf.h:
    /home/user/install/include/bpf/bpf.h

    libbpf.so:
    /home/user/install/lib/libbpf.so

    libbpf.pc:
    /home/user/install/lib/pkgconfig/libbpf.pc

    bpftool:
    /home/user/install/sbin/bpftool

This confirms that bpftool and libbpf have been correctly installed under your custom prefix directory.

**4. Install Remaining Dependencies: json-c, yaml-cpp, llvm**

**Recommended: Use Spack**

.. code-block:: bash

    git clone https://github.com/spack/spack.git
    . spack/share/spack/setup-env.sh
    spack install json-c cppyaml llvm

**If Spack is not available:**

* **LLVM:** Install via your package manager

  - Fedora/RHEL:
     .. code-block:: bash

         sudo dnf install llvm-devel

  - Ubuntu/Debian:
     .. code-block:: bash

         sudo apt-get install llvm-dev

* **json-c:** Build from source

  .. code-block:: bash

      git clone https://github.com/json-c/json-c.git
      pushd json-c
      git checkout tags/json-c-0.18-20240915 -b json-c-0.18-20240915
      mkdir build && cd build
      cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
      make -j
      make install -j
      popd

* **yaml-cpp:** Build from source

  .. code-block:: bash

      git clone https://github.com/jbeder/yaml-cpp.git
      pushd yaml-cpp
      git checkout tags/yaml-cpp-0.7.0 -b yaml-cpp-0.7.0
      mkdir build && cd build
      cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
      make -j
      make install -j
      popd

**5. Update Environment Paths**

.. code-block:: bash

    export PATH=$PREFIX/bin:$PREFIX/sbin:$PATH
    export LD_LIBRARY_PATH=$PREFIX/lib:$PREFIX/lib64:$LD_LIBRARY_PATH
    export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:$PREFIX/lib64/pkgconfig:$PKG_CONFIG_PATH

**6. (Optional) BPFTime for Userspace Tracing**

For experimental userspace eBPF support:

.. code-block:: bash

    git clone https://github.com/eunomia-bpf/bpftime.git
    pushd bpftime
    git checkout tags/v0.2.0 -b v0.2.0
    git apply $DATACRUMBS_DIR/docs/patch/bpftime-v0.2.0.patch
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
    make
    make install
    popd

Configure BPFTime environment:

.. code-block:: bash

    export BPFTIME_SHM_MEMORY_MB=10240
    export BPFTIME_MAX_FD_COUNT=128000

Build DataCrumbs with Custom Prefix
------------------------------------

With all dependencies installed under your custom prefix directory, you can now build and install DataCrumbs:

**1. Create Host Configuration**

.. code-block:: bash

    cp $DATACRUMBS_DIR/docs/example/example.yaml $DATACRUMBS_DIR/etc/datacrumbs/configs/$(hostname).yaml
    # Edit the configuration file as needed for your system

**2. Set CMake Arguments**

.. code-block:: bash

    cmake_args=(
        -DCMAKE_PREFIX_PATH=$PREFIX
        -DCMAKE_INSTALL_PREFIX=$PREFIX
        -DBPFTOOL_EXECUTABLE=$PREFIX/sbin/bpftool
        -DDATACRUMBS_HOST=$(hostname)
        -DDATACRUMBS_USER=${USER}
    )

If you want to use a custom host name or user, set them explicitly:

.. code-block:: bash

    # cmake_args+=(-DDATACRUMBS_HOST=<YOUR_HOST_NAME>)
    # cmake_args+=(-DDATACRUMBS_USER=<TARGET_USER>)

**3. Build and Install**

.. code-block:: bash

    pushd $DATACRUMBS_DIR
    mkdir -p build && cd build
    cmake "${cmake_args[@]}" ..
    make -j
    make install
    popd

Build with Environment Modules (HPC Systems)
=============================================

On HPC systems using environment modules (e.g., Tuolumne supercomputer):

.. code-block:: bash

    # Load required modules
    module load gcc/11.2.0
    export CC=$(which gcc)
    export CXX=$(which g++)

    # Create build directory
    mkdir build
    cd build

    # Configure and build
    cmake -DCMAKE_INSTALL_PREFIX=$HOME/datacrumbs-install \
          -DDATACRUMBS_HOST=$(hostname) \
          -DDATACRUMBS_USER=$USER \
          ..

    make -j$(nproc)
    make install

CMake Configuration Options
============================

All DataCrumbs scripts support common options:

- ``--verbose``: Enable detailed output
- ``--quiet``: Suppress informational messages
- ``--dry-run``: Show what would be done without executing

The following table lists all available CMake configuration options:

.. list-table:: CMake Configuration Options
   :header-rows: 1
   :widths: 35 15 50

   * - Option
     - Default
     - Description
   * - **Core Options**
     -
     -
   * - ``CMAKE_INSTALL_PREFIX``
     - ``/usr/local``
     - Installation prefix for DataCrumbs
   * - ``DATACRUMBS_HOST``
     - Auto-detected
     - Host identifier for configuration files (must have matching .yaml file in ``etc/datacrumbs/configs/``)
   * - ``DATACRUMBS_USER``
     - ``$USER``
     - User name for runtime operations
   * - ``DATACRUMBS_INSTALL_USER``
     - ``$USER``
     - User name for installation file naming (for shared installations)
   * - **Tracing Configuration**
     -
     -
   * - ``DATACRUMBS_ENABLE_OPT``
     - ``ON``
     - Enable or disable tracing functionality. Values: ``ON``, ``OFF``
   * - ``DATACRUMBS_MODE_STR``
     - ``TRACE``
     - Operation mode. Values: ``TRACE`` (full event tracing), ``PROFILE`` (sampling-based profiling)
   * - ``DATACRUMBS_TRACE_ALL_PROCESSES_OPT``
     - ``OFF``
     - Trace all processes on the system (not just target application). Values: ``ON``, ``OFF``
   * - ``DATACRUMBS_INCLUSION_PATH``
     - ``NONE``
     - Filter tracing to specific file paths (e.g., ``/scratch/data``)
   * - ``DATACRUMBS_TRACE_RINGBUF_SIZE_MB``
     - ``16``
     - Size of eBPF ring buffer in megabytes. Larger buffers reduce event loss but consume more memory
   * - ``DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS``
     - ``1000``
     - Skip events with duration below threshold (nanoseconds). Default is 1 microsecond
   * - **Profiling Configuration**
     -
     -
   * - ``DATACRUMBS_TIME_INTERVAL_NS``
     - ``1000000``
     - Sampling interval for profiling mode (nanoseconds). Default is 1 millisecond
   * - **Directory Configuration**
     -
     -
   * - ``DATACRUMBS_CONFIGURED_TRACE_DIR``
     - ``/tmp``
     - Default directory for trace output
   * - ``DATACRUMBS_CONFIGURED_LOG_DIR``
     - ``/tmp``
     - Default directory for log files
   * - ``DATACRUMBS_CONFIGURED_RUN_DIR``
     - ``NONE``
     - Directory for runtime state files (PIDs, lock files). Auto-determined if not set
   * - **Kernel Configuration**
     -
     -
   * - ``DATACRUMBS_KERNEL_VERSION``
     - Auto-detected
     - Override detected kernel version (e.g., ``5.15.0``)
   * - ``DATACRUMBS_KERNEL_HEADERS_PATH``
     - Auto-detected
     - Path to kernel headers for eBPF compilation. Default: ``/lib/modules/$(uname -r)/build``
   * - ``DATACRUMBS_KERNEL_PATH``
     - Empty
     - Path to kernel source tree (if different from headers)
   * - **Scheduler Integration**
     -
     -
   * - ``DATACRUMBS_SCHEDULER_TYPE``
     - ``FLUX``
     - Job scheduler type for multi-node support. Values: ``FLUX``, ``SLURM``, ``OPENMPI``, ``NONE``
   * - ``DATACRUMBS_SCHEDULER_JOBID_ENV_VAR``
     - ``NONE``
     - Environment variable containing job ID (e.g., ``SLURM_JOB_ID``). Auto-determined from scheduler type
   * - ``DATACRUMBS_SCHEDULER_NODES_CMD_OPT``
     - ``NONE``
     - Scheduler option for specifying node count (e.g., ``-N``). Auto-determined from scheduler type
   * - ``DATACRUMBS_SCHEDULER_PPN_CMD_OPT``
     - ``NONE``
     - Scheduler option for processes per node (e.g., ``-n``). Auto-determined from scheduler type
   * - ``DATACRUMBS_SCHEDULER_RUN_CMD``
     - ``NONE``
     - Scheduler job launch command (e.g., ``srun``). Auto-determined from scheduler type
   * - ``DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS``
     - Empty
     - Additional arguments for scheduler run command (e.g., ``--exclusive --mem=0``)
   * - **Build Control Options**
     -
     -
   * - ``DATACRUMBS_SKIP_PROBE_EXPLORING``
     - ``OFF``
     - Skip the probe exploration step during build. Values: ``ON``, ``OFF``
   * - ``DATACRUMBS_SKIP_PROBE_GENERATION``
     - ``OFF``
     - Skip the probe generation step during build. Values: ``ON``, ``OFF``
   * - ``DATACRUMBS_BUILD_ONLY``
     - ``OFF``
     - Build libraries only without installation or full setup. Values: ``ON``, ``OFF``
   * - **Debug and Development**
     -
     -
   * - ``DATACRUMBS_LOG_LEVEL_STR``
     - ``INFO``
     - Logging verbosity level. Values: ``ERROR``, ``WARN``, ``INFO``, ``DEBUG``, ``TRACE``
   * - ``DATACRUMBS_BPF_PRINT_ENABLE``
     - ``OFF``
     - Enable debug printing from eBPF programs. Values: ``ON``, ``OFF``. Warning: impacts performance!
   * - **Tool Paths**
     -
     -
   * - ``BPFTOOL_EXECUTABLE``
     - Auto-detected
     - Path to bpftool binary (e.g., ``/usr/local/bin/bpftool``). Auto-detected from ``PATH``
   * - **Compatibility Options**
     -
     -
   * - ``DATACRUMBS_BPFTIME_COMPATIBLE``
     - ``OFF``
     - Enable compatibility with bpftime userspace eBPF runtime. Values: ``ON``, ``OFF``

Configuration Examples
----------------------

**Basic configuration:**

.. code-block:: bash

    cmake -DCMAKE_INSTALL_PREFIX=/opt/datacrumbs \
          -DDATACRUMBS_HOST=myhost \
          ..

**HPC configuration with custom paths:**

.. code-block:: bash

    cmake -DCMAKE_INSTALL_PREFIX=$PREFIX \
          -DDATACRUMBS_HOST=tuolumne \
          -DDATACRUMBS_USER=$USER \
          -DDATACRUMBS_SCHEDULER_TYPE=FLUX \
          -DDATACRUMBS_CONFIGURED_TRACE_DIR=/lustre/traces \
          -DDATACRUMBS_CONFIGURED_LOG_DIR=/lustre/logs \
          -DBPFTOOL_EXECUTABLE=$PREFIX/sbin/bpftool \
          ..

**Debug configuration:**

.. code-block:: bash

    cmake -DDATACRUMBS_LOG_LEVEL_STR=DEBUG \
          -DDATACRUMBS_BPF_PRINT_ENABLE=ON \
          ..

**Profiling mode with reduced overhead:**

.. code-block:: bash

    cmake -DDATACRUMBS_MODE_STR=PROFILE \
          -DDATACRUMBS_TIME_INTERVAL_NS=10000000 \
          -DDATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS=10000 \
          ..

Important Notes
---------------

.. important::
   **DATACRUMBS_HOST Configuration File**

   A configuration file named ``<host>.yaml`` **must exist** in ``etc/datacrumbs/configs/`` for the specified hostname.
   This is the most common build error. You can:

   - Copy an existing configuration (e.g., ``lead.yaml``, ``docker.yaml``) and modify it
   - Create a new one based on the examples in ``docs/example/``
   - See the :doc:`setup` section for details on configuration file structure

.. warning::
   **Performance Impact**

   - ``DATACRUMBS_TRACE_ALL_PROCESSES_OPT=ON`` generates significantly more trace data
   - ``DATACRUMBS_BPF_PRINT_ENABLE=ON`` can severely impact performance
   - Use ``DATACRUMBS_MODE_STR=PROFILE`` for lower overhead in production

Docker Build
============

Build DataCrumbs in a Docker container for a consistent environment.

Building the Docker Image
--------------------------

.. code-block:: bash

    cd infrastructure/docker
    # Use Dockerfile for standard build, or Dockerfile.build for build-only image
    docker build -t datacrumbs:latest -f Dockerfile .

The Dockerfile performs:

1. Starts from base image with dependencies
2. Copies source code to ``/opt/datacrumbs``
3. Configures with CMake for Docker environment
4. Builds and installs DataCrumbs
5. Sets up environment variables

Running the Container
---------------------

.. code-block:: bash

    # Run with required privileges for eBPF
    docker run --privileged --cap-add=ALL \
               -v /sys/kernel/debug:/sys/kernel/debug:rw \
               -v /lib/modules:/lib/modules:ro \
               -it datacrumbs:latest bash

.. warning::
   eBPF requires ``--privileged`` and ``--cap-add=ALL`` capabilities!

Custom Docker Build
-------------------

Modify the Dockerfile for your environment:

.. code-block:: dockerfile

    # Example custom configuration
    cmake -DDATACRUMBS_HOST=myhost \
          -DDATACRUMBS_USER=myuser \
          -DDATACRUMBS_KERNEL_HEADERS_PATH=/usr/src/kernels/$(uname -r) \
          -DCMAKE_INSTALL_PREFIX=/opt/datacrumbs-install \
          /opt/datacrumbs/

Lima Build (macOS Development)
==============================

Lima provides a Linux VM environment on macOS for eBPF development.

Initial Setup
-------------

.. code-block:: bash

    # Install Lima (macOS)
    brew install lima

    # Start Lima VM with DataCrumbs configuration
    cd infrastructure/lima
    limactl start --network=lima:user-v2 --name=ebpf ebpf.yaml

The ``ebpf.yaml`` configuration:

* Creates Ubuntu 22.04 VM
* Installs all DataCrumbs dependencies
* Sets up BCC, spack, and OpenMPI
* Configures 4 CPU cores and 4GB RAM

Connecting to Lima
------------------

.. code-block:: bash

    # Connect to VM
    limactl shell ebpf

Building in Lima
----------------

Inside the Lima VM:

.. code-block:: bash

    # Set environment variables
    export DATACRUMBS_DIR=/home/lima.linux/datacrumbs
    export DATACRUMBS_INSTALL_DIR=/home/lima.linux/datacrumbs/install

    # Clone repository (if not mounted)
    git clone https://github.com/LLNL/datacrumbs.git $DATACRUMBS_DIR

    # Build
    cd $DATACRUMBS_DIR
    mkdir build
    cd build

    cmake -DCMAKE_INSTALL_PREFIX=$DATACRUMBS_INSTALL_DIR \
          -DDATACRUMBS_HOST=lima \
          -DDATACRUMBS_USER=$USER \
          ..

    make -j$(nproc)
    make install

Using Spack (Optional)
----------------------

Lima setup includes Spack for dependency management:

.. code-block:: bash

    # Inside Lima VM
    export SPACK_ROOT=/opt/spack
    source $SPACK_ROOT/share/spack/setup-env.sh

    # Install dependencies via Spack
    spack install openmpi@5.0.5
    spack load openmpi

Building in Chameleon
======================
Chameleon is a configurable experimental environment for large-scale edge to cloud research.
Only registered users by/as PIs have access Chameleon resources.

Setting up Chameleon Instance
-----------------
Pick a hardware type and site using Chameleon resource catalouge. Go to the the chosen site’s “Reservations → Leases” page and create a lease with one floating IP.
Once the lease becomes ACTIVE, open “Compute → Instances,” launch a new instance by selecting your lease.
Select UBUNTU-22.04.04_DATACRUMBS image when launching your instance and attach sharednet1 network.
Finally associate a floating IP in the “Network → Floating IPs” section, then connect from your terminal with SSH using the cc user and your private key.


Build DataCrumbs for Chameleon
------------------------------
Inside the instance:

Make sure the package config can find libbpf, you can set the PKG_CONFIG_PATH if needed:

.. code-block:: bash

    export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH

Set Installation Directory:

.. code-block:: bash

    export DATACRUMBS_DIR=/home/$USER/datacrumbs/install

Create host configuration:

.. code-block:: bash

    cp $DATACRUMBS_DIR/docs/example/example.yaml \
       $DATACRUMBS_DIR/etc/datacrumbs/configs/chameleon.yaml
    # Edit chameleon.yaml as needed for your system

Configure CMake arguments:

.. code-block:: bash
    cmake_args=(
        -DCMAKE_INSTALL_PREFIX=$DATACRUMBS_DIR
        -DDATACRUMBS_HOST=chameleon
        -DDATACRUMBS_USER=$USER
        -DDATACRUMBS_CONFIGURED_TRACE_DIR=$DATACRUMBS_DIR/traces
        -DDATACRUMBS_CONFIGURED_LOG_DIR=$DATACRUMBS_DIR/logs
    )
Build and install:

.. code-block:: bash
    pushd $DATACRUMBS_DIR
    mkdir -p build && cd build
    cmake "${cmake_args[@]}" ..
    make -j16
    make install
    popd

Running DataCrumbs on Chameleon
-------------------------------
For a standard usage (without BPFTime):

.. code-block:: bash
    # See Usage section for complete examples
    datacrumbs_server_run.sh
    datacrumbs_run --help

Building for Tuolumne Supercomputer
====================================

Tuolumne (Tuo) is an LLNL supercomputer requiring specific build procedures. This section provides complete build instructions including dependency installation with custom prefix.

Prerequisites for Tuolumne
---------------------------

Load required modules:

.. code-block:: bash

    module load gcc/11.2.0
    export CC=$(which gcc)
    export CXX=$(which g++)

Set Installation Prefix
------------------------

All dependencies and DataCrumbs will be installed under a custom prefix:

.. code-block:: bash

    export PREFIX=/usr/workspace/$USER/datacrumbs-install

Build DataCrumbs for Tuolumne
------------------------------

Follow the instructions in the :ref:`Custom Prefix Installation` section above to build and install all dependencies.

Once dependencies are installed, clone and configure DataCrumbs:

.. code-block:: bash

    git clone https://github.com/LLNL/datacrumbs.git
    export DATACRUMBS_DIR=$(realpath datacrumbs)

Create host configuration:

.. code-block:: bash

    cp $DATACRUMBS_DIR/docs/example/example.yaml \
       $DATACRUMBS_DIR/etc/datacrumbs/configs/tuolumne.yaml
    # Edit tuolumne.yaml as needed for your system

Configure CMake arguments:

.. code-block:: bash

    cmake_args=(
        -DCMAKE_PREFIX_PATH=$PREFIX
        -DCMAKE_INSTALL_PREFIX=$PREFIX
        -DBPFTOOL_EXECUTABLE=$PREFIX/sbin/bpftool
        -DDATACRUMBS_HOST=tuolumne
        -DDATACRUMBS_USER=$USER
        -DDATACRUMBS_SCHEDULER_TYPE=FLUX
        -DDATACRUMBS_CONFIGURED_TRACE_DIR=/p/lustre1/$USER/traces
        -DDATACRUMBS_CONFIGURED_LOG_DIR=/p/lustre1/$USER/logs
    )

Build and install:

.. code-block:: bash

    pushd $DATACRUMBS_DIR
    mkdir -p build && cd build
    cmake "${cmake_args[@]}" ..
    make -j16
    make install
    popd

Running DataCrumbs on Tuolumne
-------------------------------

Load environment:

.. code-block:: bash

    module load gcc/11.2.0
    export PATH=$PREFIX/bin:$PREFIX/sbin:$PATH
    export LD_LIBRARY_PATH=$PREFIX/lib:$PREFIX/lib64:$LD_LIBRARY_PATH

Run with BPFTime (if installed):

.. code-block:: bash

    export BPFTIME_SHM_MEMORY_MB=10240
    export BPFTIME_MAX_FD_COUNT=128000

    bpftime --install-location $PREFIX/lib load \
        $PREFIX/sbin/datacrumbs "run" "tuolumne-mpiio" \
        "--user" "$USER" \
        "--config_path" "$PREFIX/etc/datacrumbs/configs" \
        "--data_dir" "$PREFIX/etc/datacrumbs/data" \
        "--trace_log_dir" "$PREFIX/etc/datacrumbs/logs"

For debugging with GDB:

.. code-block:: bash

    gdb $PREFIX/sbin/datacrumbs
    # Inside GDB:
    add-auto-load-safe-path /opt/cray/pe/gcc/11.2.0/snos/lib64/libstdc++.so.6.0.29-gdb.py
    set follow-fork-mode child
    set detach-on-fork off
    set print-frame-arguments all

Standard usage (without BPFTime):

.. code-block:: bash

    # See Usage section for complete examples
    datacrumbs_server_run.sh
    datacrumbs_run --help

Build Verification
==================

After installation, verify the build:

.. code-block:: bash

    # Check binaries
    ls -la $PREFIX/bin/datacrumbs_*
    ls -la $PREFIX/sbin/datacrumbs*

    # Check libraries (may be in lib or lib64 depending on system)
    ls -la $PREFIX/lib*/libdatacrumbs_*.so

    # Check module file
    ls -la $PREFIX/etc/datacrumbs/modulefiles/

    # Test validator
    $PREFIX/sbin/datacrumbs_validator

Troubleshooting
===============

Build Errors
------------

**"Cannot find configuration file for host"**

.. code-block:: text

    CMake Error: Configuration file etc/datacrumbs/configs/myhost.yaml not found

**This is the most common build error.** DataCrumbs requires a host-specific configuration file.

.. code-block:: bash

    # Solution 1: Copy an existing configuration
    cd etc/datacrumbs/configs/
    cp lead.yaml myhost.yaml
    # Edit myhost.yaml to match your system

    # Solution 2: Specify a different host
    cmake -DDATACRUMBS_HOST=lead ...

    # Solution 3: Create minimal configuration
    cat > etc/datacrumbs/configs/myhost.yaml << 'EOF'
    capture_probes:
      - name: sys
        probe: syscalls
        type: header
        file: /usr/src/kernels/$(uname -r)/include/linux/syscalls.h
        regex: sys_.*
    EOF

See the :doc:`setup` section for more details on configuration file structure.

**"Cannot find vmlinux BTF"**

.. code-block:: bash

    # Verify BTF exists
    ls /sys/kernel/btf/vmlinux

    # If missing, specify kernel headers path
    cmake -DDATACRUMBS_KERNEL_HEADERS_PATH=/usr/src/kernels/$(uname -r) ...

**"libbpf not found"**

.. code-block:: bash

    # Install libbpf or set PKG_CONFIG_PATH
    export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH

**"Clang not found for eBPF compilation"**

.. code-block:: bash

    # Install clang
    sudo dnf install clang llvm

    # Verify
    which clang

**"Permission denied" during eBPF operations**

The build process tests eBPF capabilities. Ensure:

.. code-block:: bash

    # Run make with appropriate privileges if needed
    sudo make

Rebuild from Scratch
--------------------

To completely rebuild:

.. code-block:: bash

    # Remove build directory
    rm -rf build

    # Recreate and rebuild
    mkdir build
    cd build
    cmake ..
    make clean
    make -j$(nproc)

Partial Rebuild
---------------

To rebuild only changed components:

.. code-block:: bash

    # Rebuild specific targets
    make datacrumbs_bpf
    make datacrumbs

    # Reinstall
    make install

Clean Build Artifacts
---------------------

.. code-block:: bash

    # Clean BPF artifacts
    make clean_all

    # Clean everything
    make clean
