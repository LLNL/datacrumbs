=================
Using DataCrumbs
=================

DataCrumbs provides multiple modes of operation for tracing applications. This guide covers all usage patterns.

Quick Start
===========

**Recommended: Use Binary Tracking**

Applications should be instrumented using ``datacrumbs_track`` (recommended) or ``datacrumbs_wrap``:

.. code-block:: bash

    # Source setup
    source /path/to/install/bin/datacrumbs_setup
    
    # Recommended: Track the application (permanent instrumentation)
    datacrumbs_track --executable ./myapp
    datacrumbs_run --app "./myapp arg1 arg2"
    
    # Alternative: Wrap the application (runtime instrumentation)
    datacrumbs_run --app "datacrumbs_wrap ./myapp arg1 arg2"

This automatically:

1. Starts the DataCrumbs server
2. Runs your instrumented application with tracing enabled
3. Stops the server and collects traces

.. note::
   **Tracked applications work normally when DataCrumbs is not running** - they simply won't capture trace data. When the DataCrumbs server is running, data is automatically captured. When it's not running, the application executes normally with no tracing overhead.

Common Script Options
=====================

All DataCrumbs scripts support the following options:

.. list-table:: Common Script Options
   :header-rows: 1
   :widths: 20 80

   * - Option
     - Description
   * - ``--verbose``
     - Enable detailed output for debugging and monitoring
   * - ``--quiet``
     - Suppress informational messages (errors still shown)
   * - ``--dry-run``
     - Show what would be executed without actually running commands

Example usage:

.. code-block:: bash

    # Verbose mode to see detailed operations
    datacrumbs_run --verbose --app "./myapp"
    
    # Quiet mode for automated scripts
    datacrumbs_run --quiet --app "./batch_job"
    
    # Dry-run to preview actions
    datacrumbs_track --dry-run --executable ./myapp
    
    # Combine options
    datacrumbs_compose --action discover --verbose

Usage Modes
===========

DataCrumbs supports three primary usage modes:

1. **Wrapper Mode** (``datacrumbs_run``): Easiest - wraps application execution
2. **Server Mode** (``datacrumbs_server_run.sh``): Long-running server for multiple sessions
3. **Service Mode** (``datacrumbs_service_run.sh``): Systemd service integration

Wrapper Mode
============

Use ``datacrumbs_run`` for single application tracing sessions.

Basic Usage
-----------

.. code-block:: bash

    datacrumbs_run --app "command args"

Example: Trace a Simple Program
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    # Trace dd command
    datacrumbs_run --app "dd if=/dev/zero of=/tmp/test.dat bs=1M count=100"
    
    # Trace Python script
    datacrumbs_run --app "python3 myanalysis.py"
    
    # Trace with complex arguments
    datacrumbs_run --app "tar -czf backup.tar.gz /data/important"

MPI Applications
----------------

For MPI applications, specify node and process configuration. The MPI launcher (e.g., ``mpirun``, ``srun``, ``flux run``) is configured in the project YAML and automatically used by DataCrumbs.

.. code-block:: bash

    datacrumbs_run --app "./mpiapp input.dat" --enable_mpi --nodes 4 --ppn 8

**Options:**

- ``--enable_mpi``: Enable MPI mode
- ``--nodes N``: Number of nodes to use
- ``--ppn N``: Processes per node

.. note::
   Do not include the MPI launcher (``mpirun``, ``srun``, etc.) in the ``--app`` command. DataCrumbs automatically uses the launcher configured in ``project.yaml`` (``job.run`` setting).

Example: MPI Application
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    # Run 32 processes (4 nodes × 8 processes/node)
    datacrumbs_run --app "./myapp" --enable_mpi --nodes 4 --ppn 8
    
    # With arguments
    datacrumbs_run --app "./simulation input.dat" --enable_mpi --nodes 2 --ppn 16

Command Reference: datacrumbs_run
----------------------------------

**Synopsis:**

.. code-block:: bash

    datacrumbs_run --app "COMMAND" [OPTIONS]

**Required Arguments:**

``--app "COMMAND"``
    Application command with arguments (quoted)

**Optional Arguments:**

``--enable_mpi``
    Enable MPI support (default: disabled)

``--nodes N``
    Number of nodes for MPI execution (default: 1)

``--ppn N``
    Processes per node for MPI execution (default: 1)

**Examples:**

.. code-block:: bash

    # Single process
    datacrumbs_run --app "./myapp input.txt"
    
    # MPI with 16 processes on 2 nodes
    datacrumbs_run --app "./parallel_app" --enable_mpi --nodes 2 --ppn 8
    
    # Complex command with pipes and redirection
    datacrumbs_run --app "cat largefile.dat | ./process > output.txt"

Output Location
---------------

**Trace Files** (DFTracer format):

Traces are saved to ``$DATACRUMBS_TRACE_DIR`` with naming based on MPI mode:

.. code-block:: bash

    # Without MPI:
    $DATACRUMBS_TRACE_DIR/trace-<user>-<run_id>-<hostname>-<config>.pfw.gz
    
    # With MPI:
    $DATACRUMBS_TRACE_DIR/trace-<user>-<run_id>-<rank>-<size>-<config>.pfw.gz

**Example filenames**:

.. code-block:: text

    trace-root-12345-node01-docker.pfw.gz           # Non-MPI mode
    trace-haridev-67890-0-4-lead.pfw.gz             # MPI rank 0 of 4
    trace-haridev-67890-3-4-lead.pfw.gz             # MPI rank 3 of 4

**Log Files**:

Logs are saved to ``$DATACRUMBS_LOG_DIR``:

.. code-block:: bash

    $DATACRUMBS_LOG_DIR/datacrumbs-server-<timestamp>.log
    $DATACRUMBS_LOG_DIR/datacrumbs-client-<timestamp>.log

See :doc:`setup` Output Files section for detailed format information.

Server Mode
===========

Use server mode when you want to run multiple tracing sessions without restarting the server.

**Prerequisites**: Applications must be instrumented before the server can trace them.

Starting the Server
-------------------

.. code-block:: bash

    sudo datacrumbs_server_run.sh

The server:

- Runs in the background
- Listens for traced applications
- Collects events from all traced processes
- Writes traces continuously to ``.pfw.gz`` files

Starting with Composable Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    sudo datacrumbs_server_run.sh --composite-name myconfig

Instrumenting Applications
--------------------------

**Before** running the server, instrument your applications:

.. code-block:: bash

    # Method 1: Track binary (permanent modification)
    datacrumbs_track --executable ./myapp
    
    # Method 2: Use wrap script (runtime injection)
    # No pre-tracking needed for wrap mode

Running Applications
--------------------

With the server running and applications instrumented:

.. code-block:: bash

    # Run tracked binary
    ./myapp
    
    # Or use wrap mode
    datacrumbs_wrap ./otherapp arg1 arg2

Stopping the Server
-------------------

.. code-block:: bash

    sudo datacrumbs_server_stop.sh

This:

- Stops the eBPF programs
- Flushes remaining events
- Closes trace files
- Cleans up resources

Command Reference: datacrumbs_server_run.sh
-------------------------------------------

**Synopsis:**

.. code-block:: bash

    sudo datacrumbs_server_run.sh [OPTIONS]

**Optional Arguments:**

``--composite-name NAME``
    Use custom composable configuration (default: standard configuration)

``--enable-mpi``
    Enable MPI support

``--nodes N``
    Number of nodes (default: 1)

``--ppn N``
    Processes per node (default: 1)

**Examples:**

.. code-block:: bash

    # Start standard server
    sudo datacrumbs_server_run.sh
    
    # Start with custom configuration
    sudo datacrumbs_server_run.sh --composite-name hdf5_only
    
    # Start for MPI environment
    sudo datacrumbs_server_run.sh --enable-mpi --nodes 4 --ppn 16

Command Reference: datacrumbs_server_stop.sh
--------------------------------------------

**Synopsis:**

.. code-block:: bash

    sudo datacrumbs_server_stop.sh [OPTIONS]

**Optional Arguments:**

``--composite-name NAME``
    Stop server for specific composable configuration

**Examples:**

.. code-block:: bash

    # Stop standard server
    sudo datacrumbs_server_stop.sh
    
    # Stop specific composable server
    sudo datacrumbs_server_stop.sh --composite-name hdf5_only

Service Mode (Systemd)
=======================

Service mode integrates DataCrumbs with systemd for managed, persistent tracing.

Service Installation
--------------------

The systemd service file is installed at:

.. code-block:: text

    <install-prefix>/etc/datacrumbs/systemd/datacrumbs@.service

Starting the Service
--------------------

.. code-block:: bash

    # Enable and start service for current user
    sudo systemctl enable datacrumbs@$USER.service
    sudo systemctl start datacrumbs@$USER.service

The service:

- Starts automatically at boot
- Restarts on failure
- Logs to systemd journal
- Runs with appropriate privileges

Using Service Scripts
^^^^^^^^^^^^^^^^^^^^^

Alternatively, use the provided scripts:

.. code-block:: bash

    # Start service
    sudo datacrumbs_service_run.sh
    
    # Stop service
    sudo datacrumbs_service_stop.sh

Checking Service Status
-----------------------

.. code-block:: bash

    # Check status
    sudo systemctl status datacrumbs@$USER.service
    
    # View logs
    sudo journalctl -u datacrumbs@$USER.service -f
    
    # Check if running
    ps aux | grep datacrumbs

Stopping the Service
--------------------

.. code-block:: bash

    # Stop service
    sudo systemctl stop datacrumbs@$USER.service
    
    # Disable automatic startup
    sudo systemctl disable datacrumbs@$USER.service
    
    # Or use script
    sudo datacrumbs_service_stop.sh

Command Reference: datacrumbs_service_run.sh
--------------------------------------------

**Synopsis:**

.. code-block:: bash

    sudo datacrumbs_service_run.sh

**Description:**

Starts DataCrumbs as a systemd service. The service is automatically enabled and started.

**Prerequisites:**

- Root or sudo access
- Systemd service file installed
- Environment configured via ``datacrumbs_setup``

Command Reference: datacrumbs_service_stop.sh
---------------------------------------------

**Synopsis:**

.. code-block:: bash

    sudo datacrumbs_service_stop.sh

**Description:**

Stops the DataCrumbs systemd service.

Composable Mode
===============

Composable mode allows using custom-built DataCrumbs configurations for specialized tracing scenarios.

Creating a Composable Configuration
------------------------------------

.. code-block:: bash

    # Discover available probes
    sudo datacrumbs_compose --action discover
    
    # Build custom configuration
    sudo datacrumbs_compose --action build --name myconfig

Running with Composable
------------------------

.. code-block:: bash

    # Using run wrapper
    datacrumbs_compose_run --composite-name myconfig
    
    # Or start server with composable
    sudo datacrumbs_server_run.sh --composite-name myconfig

Command Reference: datacrumbs_compose
-------------------------------------

**Synopsis:**

.. code-block:: bash

    sudo datacrumbs_compose --action ACTION [OPTIONS]

**Arguments:**

``--action ACTION``
    Action to perform: ``discover``, ``build``, ``clean``

``--name NAME``
    Name for the composable configuration

**Examples:**

.. code-block:: bash

    # Discover available probes
    sudo datacrumbs_compose --action discover
    
    # Build custom config
    sudo datacrumbs_compose --action build --name hdf5_posix
    
    # Clean composable builds
    sudo datacrumbs_compose --action clean

Advanced Usage
==============

Filtering by Path
-----------------

**Administrator Configuration Only**

Path filtering must be configured by the system administrator at build time and cannot be changed by users at runtime.

.. code-block:: bash

    # Administrator sets inclusion path at build time:
    # cmake -DDATACRUMBS_INCLUSION_PATH=/scratch/data ..
    # make install
    #
    # Or via project YAML configuration:
    # inclusion_path: /scratch/data

Once configured, only I/O operations on files under the specified path (e.g., ``/scratch/data``) will be traced.

.. note::
   Users cannot change the inclusion path at runtime. Contact your system administrator to modify path filtering.

Multi-Node Tracing
------------------

For distributed tracing across multiple nodes:

.. code-block:: bash

    # Start server on each node (via scheduler)
    srun -N 4 sudo datacrumbs_server_run.sh --enable-mpi --nodes 4
    
    # Run application
    srun -N 4 -n 32 ./mpi_app
    
    # Stop servers
    srun -N 4 sudo datacrumbs_server_stop.sh

Traces are collected independently on each node.

Integration with Job Schedulers
================================

SLURM
-----

.. code-block:: bash

    #!/bin/bash
    #SBATCH -N 4
    #SBATCH -n 32
    #SBATCH -t 1:00:00
    
    # Load DataCrumbs
    module load datacrumbs/0.0.4
    
    # Run with tracing
    datacrumbs_run --app "./myapp" --enable_mpi --nodes 4 --ppn 8

FLUX
----

.. code-block:: bash

    #!/bin/bash
    
    # Load DataCrumbs
    module load datacrumbs/0.0.4
    
    # Submit job with tracing
    flux run -N 4 -n 32 datacrumbs_run --app "./myapp" --enable_mpi

OpenMPI (Standalone)
--------------------

.. code-block:: bash

    # Load DataCrumbs and MPI
    module load datacrumbs/0.0.4
    module load openmpi
    
    # Run with tracing (MPI launcher configured in project.yaml)
    datacrumbs_run --app "./myapp" --enable_mpi --nodes 4 --ppn 8

Trace Analysis
==============

Viewing Traces
--------------

DataCrumbs outputs traces in **DFTracer format** (``.pfw.gz`` compressed files):

.. code-block:: bash

    # Find your traces
    ls $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz
    
    # Example output:
    # trace-root-12345-node01-docker.pfw.gz
    # trace-haridev-67890-0-4-lead.pfw.gz

**Format**: DFTracer format - see https://dftracer.readthedocs.io/en/latest/trace_format.html

The trace includes:

- Timestamped events (nanosecond precision)
- Event IDs (mapped via ``categories-*.json``)
- Function entry/exit pairs
- Process/thread/rank information
- File access patterns

Analyzing Traces with DFTracer Tools
-------------------------------------

**DFTracer Utilities**

Use DFTracer utilities for trace manipulation and inspection:

.. code-block:: bash

    # Install DFTracer utilities
    pip install dftracer-utils
    
    # Get trace statistics
    dftracer-stats --input trace-*.pfw.gz
    
    # Merge multiple trace files (e.g., from MPI ranks)
    dftracer-merge --input "trace-*-67890-*.pfw.gz" --output merged-trace.pfw.gz
    
    # Split large trace files
    dftracer-split --input merged-trace.pfw.gz --output-dir split-traces/

**Documentation**: https://dftracer.readthedocs.io/en/latest/bash_utilities.html

**DFAnalyzer - Comprehensive Analysis**

For detailed analysis, use DFAnalyzer:

.. code-block:: bash

    # Install DFAnalyzer
    pip install dfanalyzer
    
    # Generate analysis report
    dfanalyzer analyze --input trace-*.pfw.gz --output report.html
    
    # Interactive analysis
    dfanalyzer interactive --input trace-*.pfw.gz
    
    # Export to CSV for custom analysis
    dfanalyzer export --input trace-*.pfw.gz --format csv --output trace-data.csv

**Documentation**: https://dftracer.readthedocs.io/projects/analyzer/en/latest/getting-started.html#usage

Custom Analysis Scripts
-----------------------

DataCrumbs also includes Jupyter notebooks for custom analysis:

.. code-block:: bash

    # Navigate to analysis directory
    cd $DATACRUMBS_INSTALL/share/datacrumbs/analysis
    
    # Install dependencies
    pip install -r requirements.txt
    
    # Run analysis notebook
    jupyter notebook analysis.ipynb

Troubleshooting
===============

Permission Denied Errors
------------------------

.. code-block:: bash

    # Ensure sudo for server operations
    sudo datacrumbs_server_run.sh
    
    # Check ulimits
    ulimit -n
    ulimit -l
    
    # Increase if needed
    ulimit -n 1048576
    ulimit -l unlimited

Server Won't Start
------------------

.. code-block:: bash

    # Check if already running
    ps aux | grep datacrumbs
    
    # Check logs
    cat $DATACRUMBS_TRACE_DIR/datacrumbs.log
    
    # Verify eBPF support
    ls /sys/kernel/btf/vmlinux
    
    # Check for port conflicts
    sudo lsof -i -P -n | grep datacrumbs

No Traces Generated
-------------------

.. code-block:: bash

    # Verify server is running
    ps aux | grep datacrumbs
    
    # Check trace directory exists
    ls -la $DATACRUMBS_TRACE_DIR
    
    # Verify application is instrumented
    ldd ./myapp | grep datacrumbs
    
    # Check logs for errors
    tail -f $DATACRUMBS_TRACE_DIR/datacrumbs.log

High Overhead
-------------

.. code-block:: bash

    # Reduce traced events
    export DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS=10000
    
    # Use profiling mode instead of tracing
    # (Administrator must rebuild with -DDATACRUMBS_MODE_STR=PROFILE)
    
    # Path filtering (Administrator only - set at build time)
    # Contact your administrator to enable path filtering

Missing Events
--------------

.. code-block:: bash

    # Increase ring buffer size
    # (Rebuild with -DDATACRUMBS_TRACE_RINGBUF_SIZE_MB=64)
    
    # Check for event drops
    grep "dropped" $DATACRUMBS_TRACE_DIR/datacrumbs.log

Best Practices
==============

1. **Use binary tracking** (``datacrumbs_track``) - recommended for most use cases; apps run normally when DataCrumbs is not active
2. **Use wrapper mode** (``datacrumbs_run``) for simplicity and automatic server management
3. **Use server mode** for multiple short-running applications
4. **Use service mode** for continuous, production monitoring
5. **Path filtering** (administrator only) - contact your admin to enable filtering by specific paths
6. **Increase ring buffer** for high-throughput I/O applications
7. **Check logs** regularly for warnings or errors
8. **Clean old traces** periodically to free disk space
9. **Test on small workloads** before production runs
10. **Use composables** for specialized tracing needs
11. **Monitor overhead** and adjust thresholds as needed

Examples
========

Example 1: Trace HDF5 Application
----------------------------------

.. code-block:: bash

    # Load environment
    module load datacrumbs/0.0.4
    module load hdf5
    
    # Run with tracing
    datacrumbs_run --app "./hdf5_writer dataset.h5"
    
    # View trace files (.pfw.gz format)
    ls -lh $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz

Example 2: Trace MPI-IO Application
------------------------------------

.. code-block:: bash

    # Load modules
    module load datacrumbs/0.0.4
    module load openmpi
    
    # Run MPI application with tracing (64 processes on 4 nodes)
    datacrumbs_run --app "./parallel_io" \
                   --enable_mpi --nodes 4 --ppn 16
    
    # View traces from all ranks (64 .pfw.gz files, one per rank)
    ls -lh $DATACRUMBS_TRACE_DIR/trace-*-*-64-*.pfw.gz

Example 3: Long-Running Server
-------------------------------

.. code-block:: bash

    # Start server
    sudo datacrumbs_server_run.sh
    
    # Track multiple applications
    datacrumbs_track --executable ./app1
    datacrumbs_track --executable ./app2
    
    # Run applications (they're automatically traced)
    ./app1 &
    ./app2 &
    
    # Wait for completion
    wait
    
    # Stop server
    sudo datacrumbs_server_stop.sh
    
    # View collected traces
    ls -lh $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz

Example 4: Filtered Tracing
----------------------------

**Prerequisite**: Administrator must have configured path filtering at build time.

.. code-block:: bash

    # If administrator configured filtering for /scratch filesystem:
    # cmake -DDATACRUMBS_INCLUSION_PATH=/scratch ..
    
    # Run application - only /scratch I/O will be traced
    datacrumbs_run --app "./mixed_io"
    
    # Only /scratch I/O operations are in the trace
    # View the generated trace file
    ls -lh $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz
