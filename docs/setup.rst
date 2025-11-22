====================
Environment Setup
====================

After building and installing DataCrumbs, you need to set up your environment to use the tools.

Setup Script Overview
=====================

DataCrumbs provides a ``datacrumbs_setup`` script that configures the environment for running tracing sessions.

Basic Setup
===========

Source the setup script to configure your environment:

.. code-block:: bash

    # If using environment modules
    module use /path/to/install/etc/datacrumbs/modulefiles
    module load datacrumbs/<version>

    # Or source directly
    source /path/to/install/bin/datacrumbs_setup

What Setup Does
===============

The setup script:

1. **Detects User Context**: Determines if running as root or regular user
2. **Loads Configuration**: Reads project and host-specific YAML configuration
3. **Sets Environment Variables**: Configures paths and runtime parameters
4. **Configures Resource Limits**: Sets appropriate ulimits for eBPF operations
5. **Creates Aliases**: Provides convenient command shortcuts
6. **Loads Module (optional)**: Integrates with environment modules if available

Environment Variables Set
=========================

After sourcing the setup script, the following variables are available:

.. list-table:: Global Configuration Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_VERSION``
     - DataCrumbs version number (e.g., ``0.0.4``)
   * - ``DATACRUMBS_LIB_VERSION``
     - Library version number (e.g., ``1.0.0``)

.. list-table:: Installation Path Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_INSTALL_HOST``
     - Hostname where DataCrumbs is installed
   * - ``DATACRUMBS_INSTALL_USER``
     - User who installed DataCrumbs
   * - ``DATACRUMBS_INSTALL_PREFIX``
     - Installation prefix directory (e.g., ``/opt/datacrumbs``)
   * - ``DATACRUMBS_INSTALL_BIN_DIR``
     - Directory for user command binaries
   * - ``DATACRUMBS_INSTALL_SBIN_DIR``
     - Directory for system/admin binaries
   * - ``DATACRUMBS_INSTALL_LIB_DIR``
     - Directory for shared libraries
   * - ``DATACRUMBS_INSTALL_LIBEXEC_DIR``
     - Directory for internal executables and eBPF objects
   * - ``DATACRUMBS_INSTALL_ETC_DIR``
     - Directory for configuration files

.. list-table:: Client Configuration Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_CLIENT_LIB``
     - Path to the client library (``libdatacrumbs_client.so``)
   * - ``DATACRUMBS_CLIENT_BIN``
     - Directory for user-specific client binaries

.. list-table:: Trace Configuration Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_TRACE_DIR``
     - Directory for trace files. Supports date patterns: ``%YY%`` (year), ``%MM%`` (month), ``%DD%`` (day)
   * - ``DATACRUMBS_TRACE_DIR_PATTERN``
     - Pattern template for trace directory (e.g., ``/scratch/traces/%YY%/%MM%/%DD%``)

.. list-table:: Job Scheduler Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_JOB_SCHEDULER``
     - Active job scheduler (``FLUX``, ``SLURM``, ``OPENMPI``, etc.)
   * - ``DATACRUMBS_JOB_RUN``
     - Command to run jobs (e.g., ``flux run``, ``srun``)
   * - ``DATACRUMBS_JOB_NODE_FLAG``
     - Flag for specifying number of nodes (e.g., ``-N``)
   * - ``DATACRUMBS_JOB_PPN_FLAG``
     - Flag for specifying processes per node (e.g., ``-n``)
   * - ``DATACRUMBS_JOB_ID_VAR``
     - Environment variable containing job ID (e.g., ``FLUX_JOB_ID``, ``SLURM_JOB_ID``)

.. list-table:: Server Configuration Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_SERVER_LOAD_TIMEOUT``
     - Timeout in seconds for server to load eBPF programs (default: ``600``)
   * - ``DATACRUMBS_SERVER_RUN_DIR``
     - Directory for server runtime files (PID files, lock files)
   * - ``DATACRUMBS_SERVER_RUN_ID``
     - Unique identifier for the current tracing session
   * - ``DATACRUMBS_SERVER_MODULE``
     - Flag for module system integration (``0`` or ``1``)

.. list-table:: Log Configuration Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_LOG_DIR``
     - Directory for log files (e.g., ``/var/log/datacrumbs``)

.. list-table:: Runtime Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_USER``
     - Current user running DataCrumbs (auto-detected or from ``$USER``)
   * - ``DATACRUMBS_IS_ROOT``
     - Flag indicating if running as root (``1``) or regular user (``0``)

.. list-table:: System Path Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``PATH``
     - Updated to include ``$PREFIX/bin`` (user commands) and ``$PREFIX/sbin`` (admin commands)
   * - ``LD_LIBRARY_PATH``
     - Updated to include ``$PREFIX/lib`` (DataCrumbs libraries) and dependency library paths

.. list-table:: Module System Variables
   :header-rows: 1
   :widths: 30 70

   * - Variable
     - Description
   * - ``DATACRUMBS_MODULE_AVAILABLE``
     - Set to ``1`` if environment modules are available, ``0`` otherwise
   * - ``DATACRUMBS_ENV_LOADED``
     - Set to ``1`` after successful setup to prevent re-initialization

Using Environment Modules
==========================

DataCrumbs integrates with environment modules (Lmod/TCL modules) for easy environment management.

Module File Location
--------------------

The module file is installed at:

.. code-block:: text

    <install-prefix>/etc/datacrumbs/modulefiles/datacrumbs/<version>.lua

Loading the Module
------------------

.. code-block:: bash

    # Add module path
    module use /path/to/install/etc/datacrumbs/modulefiles

    # Load DataCrumbs module
    module load datacrumbs/0.0.4

    # Verify loaded
    module list

The module automatically:

- Adds binaries to ``PATH``
- Adds libraries to ``LD_LIBRARY_PATH``
- Sources ``datacrumbs_setup``
- Sets up command aliases

Module Information
------------------

View module details:

.. code-block:: bash

    # Show module information
    module show datacrumbs/0.0.4

    # Show help text
    module help datacrumbs/0.0.4

Unloading the Module
--------------------

.. code-block:: bash

    module unload datacrumbs

Available Command Aliases
==========================

After setup, the following aliases are available:

Tracking Commands
-----------------

``datacrumbs_track``
^^^^^^^^^^^^^^^^^^^^

Permanently instrument a binary for tracing.

.. code-block:: bash

    datacrumbs_track --executable /path/to/myapp

``datacrumbs_untrack``
^^^^^^^^^^^^^^^^^^^^^^

Remove DataCrumbs instrumentation from a binary.

.. code-block:: bash

    datacrumbs_untrack --executable /path/to/myapp

Execution Commands
------------------

``datacrumbs_run``
^^^^^^^^^^^^^^^^^^

Run an application with DataCrumbs tracing.

.. code-block:: bash

    datacrumbs_run --app "myapp arg1 arg2"

``datacrumbs_compose_run``
^^^^^^^^^^^^^^^^^^^^^^^^^^

Run using a custom composable configuration.

.. code-block:: bash

    datacrumbs_compose_run --composite-name myconfig

Server Management
-----------------

``datacrumbs_stop``
^^^^^^^^^^^^^^^^^^^

Stop the DataCrumbs server (requires sudo).

.. code-block:: bash

    datacrumbs_stop

``datacrumbs_compose``
^^^^^^^^^^^^^^^^^^^^^^

Manage composable configurations (requires sudo).

.. code-block:: bash

    sudo datacrumbs_compose --action discover

Utility Commands
----------------

``datacrumbs_wrap``
^^^^^^^^^^^^^^^^^^^

Run a command with LD_PRELOAD-based tracing (requires sudo).

.. code-block:: bash

    datacrumbs_wrap myapp arg1 arg2

``datacrumbs_create_log_dir``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Create date-based log directory structure.

.. code-block:: bash

    datacrumbs_create_log_dir

Configuration Files
===================

DataCrumbs uses YAML configuration files to define tracing behavior.

Project Configuration
---------------------

**Location**: ``<install-prefix>/etc/datacrumbs/configs/project.yaml``

Contains project-wide settings including paths, scheduler configuration, and server parameters.

Example:

.. code-block:: yaml

    version: 0.0.4
    lib_version: 1.0.0
    install:
      host: myhostname
      user: myuser
      prefix: /opt/datacrumbs
      bin_dir: ${DATACRUMBS_INSTALL_PREFIX}/bin
      sbin_dir: ${DATACRUMBS_INSTALL_PREFIX}/sbin
      lib_dir: ${DATACRUMBS_INSTALL_PREFIX}/lib
    client:
      lib: ${DATACRUMBS_INSTALL_LIB_DIR}/libdatacrumbs_client.so
      bin: ${DATACRUMBS_INSTALL_LIBEXEC_DIR}/sbin/${DATACRUMBS_USER}
    trace:
      dir_pattern: /scratch/traces/%YY%/%MM%/%DD%
    job:
      scheduler: FLUX
      run: flux run
      node_flag: -N
      ppn_flag: -n
      id_var: FLUX_JOB_ID
    server:
      load_timeout: 600
      run_dir: /var/run/datacrumbs
      module: 0
    log:
      dir: /var/log/datacrumbs

.. _host-configuration:

Host Configuration
------------------

**Location**: ``<install-prefix>/etc/datacrumbs/configs/<hostname>.yaml``

Contains host-specific settings for probe configurations and capture rules. This file **must exist** for the hostname specified during build (``DATACRUMBS_HOST``).

.. important::
   The host configuration file is required for DataCrumbs to build successfully. Copy and modify an existing configuration file (e.g., ``lead.yaml``, ``docker.yaml``) or create a new one based on the examples in ``etc/datacrumbs/configs/``.

Capture Probes Configuration
=============================

The ``capture_probes`` section defines which functions, system calls, or events DataCrumbs should trace. Each capture probe specifies what to monitor and how to discover traceable functions.

Capture probes use different mechanisms to discover and attach to functions. The capture type determines how symbols are discovered, while the probe type determines how they are instrumented at runtime.

Common Fields
-------------

All capture probe types share these base fields inherited from the ``CaptureProbe`` class:

.. list-table::
   :header-rows: 1
   :widths: 20 15 65

   * - Field
     - Required
     - Description
   * - ``name``
     - Yes
     - Unique identifier for this probe set
   * - ``probe``
     - Yes
     - Runtime probe mechanism: ``syscalls``, ``kprobe``, ``uprobe``, ``usdt``, or ``custom``
   * - ``type``
     - Yes
     - Symbol discovery method: ``header``, ``binary``, ``ksym``, ``usdt``, or ``custom``
   * - ``regex``
     - Optional
     - Regular expression to filter discovered function names (default: ``.*`` matches all)
   * - ``enable_explorer``
     - Optional
     - Enable/disable probe discovery at build time (default: ``true``)

Capture Types Overview
----------------------

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - Capture Type
     - Discovery Method
     - Compatible Probe Types
   * - ``ksym``
     - Kernel symbol table (``/proc/kallsyms``)
     - ``kprobe`` only
   * - ``header``
     - Parse C header files with Clang
     - ``syscalls``, ``kprobe``, ``uprobe``
   * - ``binary``
     - Parse ELF binary symbols
     - ``kprobe`` (kernel modules), ``uprobe`` (user binaries)
   * - ``usdt``
     - DTRACE probes in binaries
     - ``usdt`` only
   * - ``custom``
     - User-defined plugin interface
     - ``custom`` (can use any underlying mechanism)

Kernel Symbol Capture (ksym)
-----------------------------

**Capture Type**: ``ksym`` | **Compatible Probes**: ``kprobe``

**Class**: ``KernelCaptureProbe``

Discovers kernel functions from the compiled kernel symbol table at ``/proc/kallsyms``. This method queries the running kernel for available functions with type 'T' (global) or 't' (local).

**Required Fields**:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Field
     - Description
   * - ``name``
     - Identifier for the probe set
   * - ``probe``
     - Must be ``kprobe``
   * - ``type``
     - Must be ``ksym``
   * - ``regex``
     - Pattern to match kernel symbol names from ``/proc/kallsyms``

**Advantages**:
* Discovers only functions available in the running kernel
* No need for kernel source or headers
* Fast symbol lookup

**Example - XFS Filesystem Functions**:

.. code-block:: yaml

    - name: xfs
      probe: kprobe
      type: ksym
      regex: ^xfs(?!.*cold).*

**Example - IOMAP Functions**:

.. code-block:: yaml

    - name: iomap
      probe: kprobe
      type: ksym
      regex: ^iomap(?!.*cold).*

**Example - All VFS Functions**:

.. code-block:: yaml

    - name: vfs
      probe: kprobe
      type: ksym
      regex: ^vfs_.*

Header File Capture
--------------------

**Capture Type**: ``header`` | **Compatible Probes**: ``syscalls``, ``kprobe``, ``uprobe``

**Class**: ``HeaderCaptureProbe``

Parses C header files using Clang to discover function declarations. This method extracts function signatures from header files without requiring compiled binaries.

**Required Fields**:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Field
     - Description
   * - ``name``
     - Identifier for the probe set
   * - ``probe``
     - ``syscalls``, ``kprobe``, or ``uprobe``
   * - ``type``
     - Must be ``header``
   * - ``file``
     - Path to C header file
   * - ``regex``
     - Optional pattern to filter function names
   * - ``enable_explorer``
     - Optional: Disable if header parsing is slow (default: ``true``)

**Advantages**:
* Works without compiled binaries
* Extracts complete function signatures
* Useful for system calls and kernel headers

Use Case: System Calls
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: sys
      probe: syscalls
      type: header
      file: @DATACRUMBS_KERNEL_HEADERS_PATH@/include/linux/syscalls.h
      regex: sys_.*

Use Case: Kernel Page Cache
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: os_page_cache
      probe: kprobe
      type: header
      file: @DATACRUMBS_KERNEL_HEADERS_PATH@/include/linux/pagemap.h

Use Case: Kernel Functions (Alternative to ksym)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: kernel_fs
      probe: kprobe
      type: header
      file: @DATACRUMBS_KERNEL_HEADERS_PATH@/include/linux/fs.h
      regex: (inode|dentry)_.*

Binary Symbol Capture
----------------------

**Capture Type**: ``binary`` | **Compatible Probes**: ``kprobe`` (kernel modules), ``uprobe`` (user binaries)

**Class**: ``BinaryCaptureProbe``

Extracts symbols from compiled ELF binaries using symbol table parsing. Works for both kernel modules (.ko) and user-space binaries/libraries.

**Required Fields**:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Field
     - Description
   * - ``name``
     - Identifier for the probe set
   * - ``probe``
     - ``kprobe`` (for .ko files) or ``uprobe`` (for binaries/libraries)
   * - ``type``
     - Must be ``binary``
   * - ``file``
     - Path to binary, shared library, or kernel module
   * - ``regex``
     - Pattern to filter symbol names (recommended to exclude compiler-generated names)
   * - ``include_offsets``
     - Optional: Include function offsets in output (default: ``false``)

**Advantages**:
* No source code or headers needed
* Works with any ELF binary
* Discovers actual available functions

Use Case: Kernel Modules (kprobe)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Example - Lustre Filesystem Module**:

.. code-block:: yaml

    - name: lustre
      probe: kprobe
      type: binary
      file: /usr/lib/modules/5.14.0/weak-updates/lustre/fs/lustre.ko
      regex: (?!.*__)(?!.*:)(?!.*constprop)(?!.*isra).*

**Regex Explanation**: Excludes compiler-generated names:
* ``(?!.*__)`` - Exclude functions with double underscores
* ``(?!.*:)`` - Exclude namespaced symbols
* ``(?!.*constprop)`` - Exclude const propagation variants
* ``(?!.*isra)`` - Exclude interprocedural scalar replacement variants

**Example - Lustre OSC Module**:

.. code-block:: yaml

    - name: osc
      probe: kprobe
      type: binary
      file: /usr/lib/modules/5.14.0/weak-updates/lustre/fs/osc.ko
      regex: (?!.*__)(?!.*:)(?!.*constprop)(?!.*isra).*

Use Case: User-Space Libraries (uprobe)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Example - Standard C Library**:

.. code-block:: yaml

    - name: libc
      probe: uprobe
      type: binary
      file: @DATACRUMBS_LIBC_SO@
      regex: .*(open|close|read|write|fork).*

**Example - MPI Library**:

.. code-block:: yaml

    - name: mpi
      probe: uprobe
      type: binary
      file: /opt/openmpi/lib/libmpi.so
      regex: MPI_File_.*

**Example - HDF5 Library**:

.. code-block:: yaml

    - name: hdf5
      probe: uprobe
      type: binary
      file: /usr/lib64/libhdf5.so.200
      regex: H5[FD].*

Use Case: Application Binaries (uprobe)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Example - IOR Benchmark**:

.. code-block:: yaml

    - name: ior
      probe: uprobe
      type: binary
      file: /home/user/software/ior/bin/ior
      regex: .*

USDT Capture
------------

**Capture Type**: ``usdt`` | **Compatible Probes**: ``usdt``

**Class**: ``USDTCaptureProbe``

Discovers User-level Statically Defined Tracing (USDT) probes using the DTRACE mechanism. USDT probes are embedded in applications and libraries for tracing, commonly used in interpreted languages like Python.

**Required Fields**:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Field
     - Description
   * - ``name``
     - Identifier for the probe set
   * - ``probe``
     - Must be ``usdt``
   * - ``type``
     - Must be ``usdt``
   * - ``binary_path``
     - Path to binary containing USDT probes
   * - ``provider``
     - USDT provider name (namespace for probes)
   * - ``regex``
     - Optional pattern to filter probe names

**Advantages**:
* Low overhead for interpreted languages
* Standardized tracing interface
* Application-specific semantic probes

**Currently Supported**: Python

**Example - Python USDT Probes**:

.. code-block:: yaml

    - name: python
      probe: usdt
      type: usdt
      binary_path: /lib64/libpython3.6m.so.1.0
      provider: python

**Example - Python GC Probes**:

.. code-block:: yaml

    - name: python_gc
      probe: usdt
      type: usdt
      binary_path: /lib64/libpython3.6m.so.1.0
      provider: python
      regex: gc.*

Custom Capture
--------------

**Capture Type**: ``custom`` | **Compatible Probes**: ``custom``

**Class**: ``CustomCaptureProbe``

Provides a plugin interface for user-defined probe discovery, function mapping, and event processing. Custom probes can work with any underlying probe mechanism (kprobe, uprobe, etc.) but with complete control over probe logic.

**Required Fields**:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Field
     - Description
   * - ``name``
     - Identifier for the custom probe
   * - ``probe``
     - Must be ``custom``
   * - ``type``
     - Must be ``custom``
   * - ``file``
     - Path to custom BPF source file (.bpf.c) with probe implementations
   * - ``probes``
     - Path to JSON file defining probe points and function mappings
   * - ``process_header``
     - Path to C header file with event processing functions
   * - ``start_event_id``
     - Starting event ID for this probe set (must be unique, typically 100000+)
   * - ``event_type``
     - Event classification: ``1`` (duration events), ``2`` (instant events)

**Advantages**:
* Complete control over probe behavior
* Custom event processing logic
* Can combine multiple probe types
* Plugin architecture for extensibility

**Use Cases**:
* Complex multi-probe workflows
* Custom data collection beyond function tracing
* Specialized performance analysis
* Domain-specific instrumentation

**Example - Custom System I/O Probe**:

.. code-block:: yaml

    - name: custom1
      start_event_id: 100000
      probe: custom
      type: custom
      file: /opt/datacrumbs/etc/datacrumbs/plugins/custom_probes/sys_io/sysio.bpf.c
      probes: /opt/datacrumbs/etc/datacrumbs/plugins/custom_probes/sys_io/probes.json
      process_header: /opt/datacrumbs/etc/datacrumbs/plugins/custom_probes/sys_io/sysio_process.h
      event_type: 2

**Plugin Structure**:

* **BPF file** (``file``): Contains eBPF programs with probe logic
* **Probes JSON** (``probes``): Defines which functions to attach to and their metadata
* **Process header** (``process_header``): C header with functions to process events in user-space

CMake Variable Substitution
============================

Configuration files support CMake variable substitution at build time:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Variable
     - Expands To
   * - ``@DATACRUMBS_PROJECT_PATH@``
     - Build directory path
   * - ``@DATACRUMBS_LIBC_SO@``
     - Detected libc library path
   * - ``@DATACRUMBS_KERNEL_HEADERS_PATH@``
     - Kernel headers directory
   * - ``@DATACRUMBS_KERNEL_UNAME_R@``
     - Kernel release version

**Example Using Variables**:

.. code-block:: yaml

    - name: libc
      probe: uprobe
      type: binary
      file: @DATACRUMBS_LIBC_SO@
      regex: .*(open|close|read|write).*

    - name: page_cache
      probe: kprobe
      type: header
      file: @DATACRUMBS_KERNEL_HEADERS_PATH@/include/linux/pagemap.h

Choosing the Right Capture Type
================================

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Capture Type
     - When to Use
     - Limitations
   * - **ksym**
     - Kernel functions only; fast discovery; no headers needed
     - Linux kernel only; no source information
   * - **header**
     - Have header files; need function signatures; build-time discovery
     - Requires header files; slower parsing
   * - **binary**
     - Have compiled binary; no source; runtime symbol discovery
     - No function signatures; binary must have symbols
   * - **usdt**
     - Tracing interpreted languages; application-specific probes
     - Limited language support (Python)
   * - **custom**
     - Complex instrumentation; custom logic; plugin development
     - Requires custom BPF programming

Complete Configuration Examples
================================

Available example configurations in ``etc/datacrumbs/configs/``:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - File
     - Description
   * - ``lead.yaml``
     - Full HPC cluster: custom probes, syscalls, Lustre modules, MPI, kernel symbols
   * - ``docker.yaml``
     - Container environment: custom probes, libc uprobe, Python USDT, kernel headers
   * - ``corona.yaml``
     - LLNL Corona system with comprehensive instrumentation
   * - ``tuolumne.yaml``
     - LLNL Tuolumne system focused on MPI-IO workflows
   * - ``hdf.yaml``
     - HDF5-focused configuration for scientific applications

Data Files
==========

DataCrumbs generates and maintains several data files during build and runtime to manage probe discovery, validation, and event mapping. These files use the naming pattern ``<filename>-<host>-<user>.<ext>`` where:

* ``<host>`` - Hostname from project configuration
* ``<user>`` - Install user from project configuration

File Locations
--------------

All data files are stored in: ``<install-prefix>/etc/datacrumbs/data/``

Probe Discovery Files
---------------------

probes-HOST-USER.json
^^^^^^^^^^^^^^^^^^^^^

**Generated**: Build time (explorer phase)

**Purpose**: Master list of discovered probe points organized by capture probe configuration

Contains all functions discovered by the explorer during the build process. Each entry represents a complete capture probe configuration with its discovered functions.

**Structure**: Array of capture probe objects, where each object contains:

* ``type`` - Probe type (0=syscalls, 1=kprobe, 2=uprobe, 3=usdt, 4=custom)
* ``name`` - Capture probe name from configuration
* ``functions`` - Array of discovered function names
* Additional fields specific to probe type (binary_path, provider, bpf_path, etc.)

**Example Entry (uprobe)**:

.. code-block:: json

    {
      "type": 2,
      "name": "libc",
      "functions": ["__GI___fork", "__GI___register_atfork", "__GI___vfork"],
      "binary_path": "/usr/lib64/libc.so.6",
      "function_offsets": ["0xf96b8", "0x841c0", "0x73458"]
    }

**Example Entry (kprobe)**:

.. code-block:: json

    {
      "type": 1,
      "name": "xfs",
      "functions": ["xfs_file_read_iter", "xfs_file_write_iter", "xfs_file_open"]
    }

**Example Entry (custom)**:

.. code-block:: json

    {
      "type": 4,
      "name": "custom1",
      "functions": ["openat", "read", "write", "close"],
      "bpf_path": "/opt/datacrumbs/etc/datacrumbs/plugins/custom_probes/sys_io/sysio.bpf.c",
      "start_event_id": 100000,
      "process_header": "/opt/datacrumbs/etc/datacrumbs/plugins/custom_probes/sys_io/sysio_process.h",
      "event_type": 2
    }

probes-exclusion-HOST-USER.json
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Managed by**: User (manual editing)

**Purpose**: User-defined probe exclusion list

Contains exclusion lists for each capture probe. Only three fields are used: ``type``, ``name``, and ``functions``. All other fields (binary_path, bpf_path, etc.) are ignored.

**Structure**: Array of objects with required fields:

* ``type`` - Probe type (must match capture probe type)
* ``name`` - Capture probe name (must match configuration)
* ``functions`` - Array of function names to exclude

**Example**:

.. code-block:: json

    [
      {
        "type": 0,
        "name": "sys",
        "functions": ["bdflush", "fork", "ioctl", "ni_syscall"]
      },
      {
        "type": 2,
        "name": "libc",
        "functions": ["fnmatch@@GLIBC_2.2.5", "sysctl@GLIBC_2.2.5"]
      },
      {
        "type": 1,
        "name": "xfs",
        "functions": ["xfs_buf_cold", "xfs_attr_check_namespace"]
      }
    ]

**Use Cases**:
* Exclude functions that cause issues or crashes
* Reduce probe count for performance
* Filter out noisy or irrelevant functions
* Avoid versioned symbols that may not exist on all systems

**Note**: The exclusion file may contain additional fields from copying the probes file, but only ``type``, ``name``, and ``functions`` are processed during probe discovery.

categories-HOST-USER.json
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Generated**: Build time (generator phase)

**Purpose**: Event ID to probe information mapping

Maps unique event IDs to their corresponding capture probe categories and function names. This is the key lookup table for trace analysis, generated during the same phase as eBPF object compilation.

**Structure**: Object with event IDs as keys, each mapping to probe metadata

**Example Structure**:

.. code-block:: json

    {
      "1000": {
        "probe_name": "libc",
        "function_name": "__GI___fork"
      },
      "1010": {
        "probe_name": "os_page_cache",
        "function_name": "add_to_page_cache_lru"
      },
      "100000": {
        "probe_name": "custom1",
        "function_name": "openat"
      }
    }

**Event ID Ranges**:
* **1000-99999**: Standard probes (kprobe, uprobe, syscalls, usdt)
* **100000+**: Custom probes (user-defined via ``start_event_id``)

This mapping enables:
* Event ID → Function name resolution during trace analysis
* Category grouping for aggregated analysis
* Human-readable trace output

manual-probes-HOST-USER.json
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Generated**: Build time (generator phase)

**Purpose**: Probes that cannot be compiled into the eBPF object files

Contains probes discovered during the generator phase that cannot be statically compiled into eBPF objects. These require runtime resolution and attachment.

**Structure**: Array of capture probe objects (same as probes file)

**Common Cases**:
* **Multi-offset symbols**: Functions with multiple addresses due to compiler optimizations or inlining
* **Dynamic libraries**: Symbols only available when library is loaded at runtime
* **Runtime-specific paths**: Kernel modules loaded after boot
* **Unresolvable symbols**: Functions that exist but cannot be compiled into static eBPF programs

**Example Entry (multiple offsets)**:

.. code-block:: json

    [
      {
        "type": 2,
        "name": "ior",
        "functions": ["2060", "2061"],
        "binary_path": "/home/user/software/ior/bin/ior",
        "include_offsets": false
      }
    ]

**Note**: The numbers in the ``functions`` array are **event IDs** that correspond to entries in ``categories-HOST-USER.json``. These event IDs reference the actual function names and metadata. These probes are attached dynamically when the DataCrumbs server starts.

probes-invalid-HOST-USER.json
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Generated**: Runtime (by ``datacrumbs_validator``)

**Purpose**: Validated probe blacklist - tracks probes that failed validation

Maintains the same structure as other probe files but lists functions deemed invalid during validation.

**Structure**: Array of capture probe objects with invalid function lists

Probes are marked invalid when:
* Attachment fails during validation
* Probe causes system instability  
* Symbol no longer exists in current kernel/binary
* Function signature incompatible with tracing

**Example**:

.. code-block:: json

    [
      {
        "type": 1,
        "name": "lustre",
        "functions": ["ll_dom_readpage", "lustre_exit", "lustre_init"]
      },
      {
        "type": 1,
        "name": "os_page_cache",
        "functions": ["page_cache_sync_readahead"]
      }
    ]

Run ``datacrumbs_validator`` after system updates or configuration changes to refresh this list.

Key Artifacts for Analysis
---------------------------

The two most important files for understanding your current tracing configuration:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - File
     - Purpose
   * - ``probes-HOST-USER.json``
     - Complete list of active probes organized by capture probe name with all discovered functions
   * - ``categories-HOST-USER.json``
     - Event ID → (probe name, function name) mapping for decoding trace events

**Workflow Integration**:

1. **Build time - Explorer phase**: Generates ``probes-*.json`` from capture_probes config
2. **User customization**: Edit ``probes-exclusion-*.json`` to exclude unwanted probes
3. **Build time - Generator phase**: Creates ``categories-*.json`` (event ID mappings) and ``manual-probes-*.json`` (probes that cannot be compiled)
4. **Runtime - Validation**: Failed probes recorded in ``probes-invalid-*.json`` by validator
5. **Runtime - Server startup**: Manual probes from step 3 are dynamically attached using event IDs from categories file
6. **Analysis**: Use ``categories-*.json`` to decode event IDs in trace files

File Management
---------------

**Regenerating Data Files**:

.. code-block:: bash

    # Rebuild to regenerate probes and categories
    cd build && make install

**Viewing Active Probes by Category**:

.. code-block:: bash

    # Show probe counts per capture probe
    jq -r '.[] | "\(.name): \(.functions | length) functions"' \
      /opt/datacrumbs/etc/datacrumbs/data/probes-*.json

**Checking Event ID Assignments**:

.. code-block:: bash

    # View event ID mappings
    jq '.' /opt/datacrumbs/etc/datacrumbs/data/categories-*.json

**Finding Specific Function's Event ID**:

.. code-block:: bash

    # Search for function in categories
    jq -r 'to_entries[] | select(.value.function_name=="xfs_file_read_iter") | 
      "Event ID: \(.key), Probe: \(.value.probe_name)"' \
      /opt/datacrumbs/etc/datacrumbs/data/categories-*.json

Output Files
============

DataCrumbs generates trace and log files during runtime. Understanding the naming conventions and formats is essential for trace analysis.

Trace Files
-----------

**Format**: DFTracer format (``.pfw.gz``)

**Documentation**: https://dftracer.readthedocs.io/en/latest/trace_format.html

Trace files are compressed binary files containing timestamped events captured by eBPF probes. Each trace file represents the activity of a single process or MPI rank.

**Naming Convention**:

The trace file naming convention depends on whether MPI is enabled:

**Without MPI** (``--disable-mpi``):

.. code-block:: text

    trace-<user>-<run_id>-<hostname>-<config_name>.pfw.gz

**Example**:

.. code-block:: text

    trace-root-12345-node01-docker.pfw.gz

**With MPI**:

.. code-block:: text

    trace-<user>-<run_id>-<mpi_rank>-<mpi_size>-<config_name>.pfw.gz

**Example**:

.. code-block:: text

    trace-haridev-67890-0-4-lead.pfw.gz
    trace-haridev-67890-1-4-lead.pfw.gz
    trace-haridev-67890-2-4-lead.pfw.gz
    trace-haridev-67890-3-4-lead.pfw.gz

**Filename Components**:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Component
     - Description
   * - ``<user>``
     - Runtime user (from ``DATACRUMBS_USER`` or ``--user``)
   * - ``<run_id>``
     - Unique run identifier (from ``--run_id`` argument)
   * - ``<hostname>``
     - System hostname (non-MPI mode only)
   * - ``<mpi_rank>``
     - MPI process rank (0 to N-1, MPI mode only)
   * - ``<mpi_size>``
     - Total number of MPI processes (MPI mode only)
   * - ``<config_name>``
     - Configuration name from command line (e.g., ``lead``, ``docker``)

**Location**:

Trace files are written to the directory specified by ``DATACRUMBS_TRACE_DIR`` (or ``--trace_log_dir`` argument).

**Default**: ``/var/log/datacrumbs`` or configured ``trace_log_dir`` from YAML.

**Analysis**:

Trace files can be analyzed using **DFTracer tools**:

**DFTracer Utilities** - Trace manipulation and statistics:

.. code-block:: bash

    pip install dftracer-utils
    dftracer-stats --input trace-*.pfw.gz
    dftracer-merge --input "trace-*-67890-*.pfw.gz" --output merged.pfw.gz
    dftracer-split --input merged.pfw.gz --output-dir split-traces/

**Documentation**: https://dftracer.readthedocs.io/en/latest/bash_utilities.html

**DFAnalyzer** - Comprehensive analysis:

.. code-block:: bash

    pip install dfanalyzer
    dfanalyzer analyze --input trace-*.pfw.gz --output report.html
    dfanalyzer interactive --input trace-*.pfw.gz

**Documentation**: https://dftracer.readthedocs.io/projects/analyzer/en/latest/getting-started.html#usage

The DFTracer format includes:

* Event timestamps (nanosecond precision)
* Event IDs (mapped via ``categories-*.json``)
* Process/thread information
* Function entry/exit pairs (for duration events)
* Custom event data (for instant events)

Log Files
---------

**Format**: Plain text (``.log``)

Log files contain human-readable diagnostic and debugging information from the DataCrumbs server and client components.

**Naming Convention**:

.. code-block:: text

    datacrumbs-<component>-<timestamp>.log

**Example**:

.. code-block:: text

    datacrumbs-server-20250122-143052.log
    datacrumbs-client-20250122-143105.log

**Location**:

Log files are written to the directory specified by ``DATACRUMBS_LOG_DIR`` (or ``--log_dir`` argument).

**Default**: Current working directory or configured ``log_dir``.

**Log Levels**:

* ``ERROR``: Critical errors requiring attention
* ``WARN``: Warnings about potential issues
* ``INFO``: General information about execution
* ``DEBUG``: Detailed debugging information
* ``TRACE``: Fine-grained execution traces

**Viewing Logs**:

.. code-block:: bash

    # View recent server logs
    tail -f /var/log/datacrumbs/datacrumbs-server-*.log

    # Search for errors
    grep ERROR /var/log/datacrumbs/*.log

    # Filter by component
    grep "\[ConfigurationManager\]" /var/log/datacrumbs/*.log

Output Management
-----------------

**Disk Space Considerations**:

* Trace files can grow large in high-frequency tracing scenarios
* Compression (``.gz``) reduces file size by ~10x
* Plan storage capacity based on trace duration and probe count

**Cleanup**:

.. code-block:: bash

    # Remove old traces (older than 7 days)
    find $DATACRUMBS_TRACE_DIR -name "trace-*.pfw.gz" -mtime +7 -delete

    # Archive traces before cleanup
    tar czf traces-archive-$(date +%Y%m%d).tar.gz $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz

**Aggregating MPI Traces**:

For MPI applications, use DFTracer utilities to merge traces from all ranks:

.. code-block:: bash

    # Install DFTracer utilities
    pip install dftracer-utils
    
    # Merge all ranks for a specific run
    dftracer-merge --input "$DATACRUMBS_TRACE_DIR/trace-*-67890-*.pfw.gz" \
                   --output "$DATACRUMBS_TRACE_DIR/merged-67890.pfw.gz"
    
    # Split large merged trace if needed
    dftracer-split --input "$DATACRUMBS_TRACE_DIR/merged-67890.pfw.gz" \
                   --output-dir "$DATACRUMBS_TRACE_DIR/split-67890/"
    
    # Analyze merged trace
    pip install dfanalyzer
    dfanalyzer analyze --input "$DATACRUMBS_TRACE_DIR/merged-67890.pfw.gz" \
                       --output "$DATACRUMBS_TRACE_DIR/report-67890.html"

**Documentation**: 

* DFTracer utilities: https://dftracer.readthedocs.io/en/latest/bash_utilities.html
* DFAnalyzer: https://dftracer.readthedocs.io/projects/analyzer/en/latest/getting-started.html#usage

Resource Limits
===============

DataCrumbs automatically configures system resource limits for optimal eBPF operation:

File Descriptors
----------------

.. code-block:: bash

    # Automatically set to maximum
    ulimit -n $(ulimit -H -n)

eBPF requires many file descriptors for maps, programs, and ring buffers.

Locked Memory
-------------

.. code-block:: bash

    # Automatically set to maximum
    ulimit -l $(ulimit -H -l)

eBPF maps and ring buffers use locked memory that cannot be swapped.

Manual Adjustment
-----------------

If automatic limits are insufficient:

.. code-block:: bash

    # Increase hard limits (requires root)
    sudo bash -c "echo '* hard nofile 1048576' >> /etc/security/limits.conf"
    sudo bash -c "echo '* soft nofile 1048576' >> /etc/security/limits.conf"

    # For current session
    ulimit -n 1048576
    ulimit -l unlimited

Directory Structure After Setup
================================

DataCrumbs creates the following directory structure:

.. code-block:: text

    <install-prefix>/
    ├── bin/                          # User commands
    │   ├── datacrumbs_setup
    │   ├── datacrumbs_run
    │   ├── datacrumbs_track
    │   ├── datacrumbs_untrack
    │   ├── datacrumbs_wrap
    │   └── ...
    ├── sbin/                         # Admin commands
    │   ├── datacrumbs                # Main server binary
    │   ├── datacrumbs_server_run.sh
    │   ├── datacrumbs_server_stop.sh
    │   ├── datacrumbs_service_run.sh
    │   ├── datacrumbs_service_stop.sh
    │   └── ...
    ├── lib/                          # Libraries
    │   ├── libdatacrumbs_client.so
    │   ├── libdatacrumbs_obj.so
    │   └── ...
    ├── libexec/                      # Internal components
    │   └── datacrumbs/
    │       ├── objects/              # eBPF object files
    │       └── composable/           # Composable configurations
    └── etc/
        └── datacrumbs/
            ├── configs/              # YAML configuration files
            ├── data/                 # Probe metadata
            ├── modulefiles/          # Environment module files
            ├── systemd/              # Systemd service files
            └── flux/                 # Flux plugin files

Verification
============

Verify your setup is correct:

.. code-block:: bash

    # Check environment variables
    echo $DATACRUMBS_USER
    echo $DATACRUMBS_TRACE_DIR
    echo $PATH | grep datacrumbs

    # Check resource limits
    ulimit -n
    ulimit -l

    # Check available commands
    which datacrumbs_run
    which datacrumbs_track

    # Test validator
    datacrumbs_validator

Multiple User Setup
===================

For shared installations where multiple users will use DataCrumbs:

Build Configuration
-------------------

.. code-block:: bash

    # Build with shared install user
    cmake -DCMAKE_INSTALL_PREFIX=/opt/datacrumbs \
          -DDATACRUMBS_INSTALL_USER=shared \
          ..

Each User Setup
---------------

.. code-block:: bash

    # Each user loads the module
    module use /opt/datacrumbs/etc/datacrumbs/modulefiles
    module load datacrumbs/0.0.4

    # User-specific trace directory
    export DATACRUMBS_TRACE_DIR=/scratch/$USER/traces

Per-User Configuration
----------------------

Users can override some default settings:

.. code-block:: bash

    # Custom log directory (if administrator enabled this)
    export DATACRUMBS_CONFIGURED_LOG_DIR=/custom/logs

.. note::
   **Trace directory cannot be overridden at runtime**. The trace directory (``DATACRUMBS_TRACE_DIR``) is set by the administrator during installation via the ``trace_dir`` or ``trace_dir_pattern`` configuration in the project YAML file. Users cannot change this location.

Troubleshooting
===============

Setup Script Not Found
-----------------------

.. code-block:: bash

    # Verify installation
    ls /path/to/install/bin/datacrumbs_setup

    # Check PATH
    export PATH=/path/to/install/bin:$PATH

Permission Issues
-----------------

.. code-block:: bash

    # Ensure directories are accessible
    ls -la $DATACRUMBS_TRACE_DIR
    ls -la $DATACRUMBS_SERVER_RUN_DIR

    # Create if needed
    mkdir -p $DATACRUMBS_TRACE_DIR
    mkdir -p $DATACRUMBS_SERVER_RUN_DIR

Module Not Found
----------------

.. code-block:: bash

    # Verify module path
    module use /path/to/install/etc/datacrumbs/modulefiles

    # List available modules
    module avail datacrumbs

Resource Limit Errors
---------------------

.. code-block:: bash

    # Check current limits
    ulimit -a

    # Increase if needed (as root)
    sudo pam_limits.so

Configuration File Errors
-------------------------

.. code-block:: bash

    # Verify configuration files exist
    ls $PREFIX/etc/datacrumbs/configs/

    # Check YAML syntax
    python3 -c "import yaml; yaml.safe_load(open('config.yaml'))"
