===============
Running DataCrumbs
===============

This guide describes how to run DataCrumbs tests and trace applications, including examples with IOR benchmarking.

Basic Test Execution
====================

Running Tests with CTest
------------------------

1. **Navigate to the build directory:**

    .. code-block:: bash

        cd $DATACRUMBS_DIR/build

2. **List all available tests:**

    .. code-block:: bash

        ctest -N

3. **View arguments for the DataCrumbs start test (Daemonize script):**

    .. code-block:: bash

        ctest -R datacrumbs_start -VV

    This will show verbose output and the arguments used for the ``datacrumbs_start`` test.

4. **View arguments for the DataCrumbs run test (sync script):**

    .. code-block:: bash

        ctest -R datacrumbs_run -VV

    This will show verbose output and the arguments used for the ``datacrumbs_run`` test.

5. **Run the ``test_simple_dd`` test:**

    .. code-block:: bash

        ctest -R test_simple_dd -VV

    This will execute the ``test_simple_dd`` test with verbose output from the build directory.

Running Applications with IOR
==============================

IOR (Interleaved Or Random) is a parallel file system benchmarking tool commonly used in HPC environments. This section demonstrates how to install and run IOR with DataCrumbs tracing.

Installing IOR
--------------

To install IOR, follow these steps:

1. **Clone the IOR repository:**

    .. code-block:: bash

        git clone https://github.com/hpc/ior.git
        cd ior

2. **Build IOR:**

    .. code-block:: bash

        ./bootstrap
        ./configure --prefix=$PREFIX
        make -j
        make install

3. **(Optional) Install system-wide:**

    .. code-block:: bash

        sudo make install

For more details, refer to the official IOR documentation: https://github.com/hpc/ior

Running IOR Benchmarks
----------------------

**Basic IOR Test:**

.. code-block:: bash

    # Run a simple IOR test
    ior -a POSIX -b 1m -t 256k -s 16 -F

This runs IOR with:
- POSIX I/O API
- 1MB block size
- 256KB transfer size
- 16 segments per task
- File-per-process mode

**Parallel IOR with MPI:**

.. code-block:: bash

    # Run IOR with MPI across multiple processes
    mpirun -np 4 ior -a MPIIO -b 4m -t 1m -s 8 -o /scratch/testfile

This runs IOR with:
- MPI-IO API
- 4 MPI processes
- 4MB block size
- 1MB transfer size
- 8 segments per task
- Output file at ``/scratch/testfile``

Tracing IOR with DataCrumbs
----------------------------

**Prerequisites**: Instrument IOR before tracing:

.. code-block:: bash

    # Track IOR binary (one-time setup)
    datacrumbs_track --executable $(which ior)
    
    # Or use wrap mode (no pre-tracking needed)

**Method 1: Using datacrumbs_run (Synchronous)**

.. code-block:: bash

    # Trace a single-node IOR run (if IOR is tracked)
    datacrumbs_run --app-name ior-test -- ior -a POSIX -b 1m -t 256k -s 16 -F
    
    # Or with wrap mode
    datacrumbs_run --app-name ior-test -- datacrumbs_wrap ior -a POSIX -b 1m -t 256k -s 16 -F

**Method 2: Using datacrumbs_server (Daemon Mode)**

.. code-block:: bash

    # Start DataCrumbs daemon
    datacrumbs_server_run.sh

    # Run your IOR benchmark
    mpirun -np 8 ior -a MPIIO -b 4m -t 1m -s 8 -o /scratch/testfile

    # Stop DataCrumbs daemon
    datacrumbs_server_stop.sh

**Method 3: Multi-node IOR with Flux**

.. code-block:: bash

    # Using Flux scheduler for multi-node execution
    flux run -N 4 -n 16 datacrumbs_run --app-name ior-multinode -- \
        ior -a MPIIO -b 8m -t 2m -s 16 -o /lustre/testfile

**Method 4: Multi-node IOR with SLURM**

.. code-block:: bash

    # Using SLURM scheduler for multi-node execution
    srun -N 4 -n 16 datacrumbs_run --app-name ior-multinode -- \
        ior -a MPIIO -b 8m -t 2m -s 16 -o /lustre/testfile

IOR Test Examples with DataCrumbs
----------------------------------

**Example 1: Write Performance Test**

.. code-block:: bash

    # Test write performance with different block sizes
    datacrumbs_run --app-name ior-write-1m -- ior -a POSIX -w -b 1m -t 256k -s 32 -F
    datacrumbs_run --app-name ior-write-4m -- ior -a POSIX -w -b 4m -t 1m -s 32 -F
    datacrumbs_run --app-name ior-write-16m -- ior -a POSIX -w -b 16m -t 4m -s 32 -F

**Example 2: Read Performance Test**

.. code-block:: bash

    # Test read performance (requires existing files from write test)
    datacrumbs_run --app-name ior-read-1m -- ior -a POSIX -r -b 1m -t 256k -s 32 -F

**Example 3: Collective I/O Test**

.. code-block:: bash

    # Test MPI-IO collective operations
    mpirun -np 4 datacrumbs_run --app-name ior-collective -- \
        ior -a MPIIO -b 4m -t 1m -s 16 -c -o /scratch/collective-test

The ``-c`` flag enables collective I/O operations.

**Example 4: Random I/O Test**

.. code-block:: bash

    # Test random I/O patterns
    datacrumbs_run --app-name ior-random -- \
        ior -a POSIX -b 1m -t 64k -s 64 -z -o /scratch/random-test

The ``-z`` flag enables random I/O instead of sequential.

**Example 5: Shared File Test**

.. code-block:: bash

    # All processes access a single shared file
    mpirun -np 8 datacrumbs_run --app-name ior-shared -- \
        ior -a MPIIO -b 2m -t 512k -s 16 -o /scratch/shared-file

Without the ``-F`` flag, all processes use a single shared file.

Analyzing IOR Traces
--------------------

After running IOR with DataCrumbs, analyze the generated traces:

.. code-block:: bash

    # View trace files in DFTracer format (.pfw.gz)
    ls -lh $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz
    
    # Example output:
    # trace-root-12345-node01-docker.pfw.gz
    # trace-haridev-67890-0-4-lead.pfw.gz  (MPI rank 0)
    # trace-haridev-67890-1-4-lead.pfw.gz  (MPI rank 1)

**Using DFTracer Utilities**:

.. code-block:: bash

    # Install DFTracer utilities
    pip install dftracer-utils
    
    # Merge traces from all MPI ranks
    dftracer-merge --input "trace-*-67890-*.pfw.gz" --output ior-merged.pfw.gz
    
    # Get I/O statistics
    dftracer-stats --input ior-merged.pfw.gz
    
    # Split large trace for easier handling
    dftracer-split --input ior-merged.pfw.gz --output-dir ior-traces/

**Documentation**: https://dftracer.readthedocs.io/en/latest/bash_utilities.html

**Using DFAnalyzer for Detailed Analysis**:

.. code-block:: bash

    # Install DFAnalyzer
    pip install dfanalyzer
    
    # Generate comprehensive report
    dfanalyzer analyze --input ior-merged.pfw.gz --output ior-report.html
    
    # Interactive analysis session
    dfanalyzer interactive --input ior-merged.pfw.gz

**Documentation**: https://dftracer.readthedocs.io/projects/analyzer/en/latest/getting-started.html#usage

**Trace Format**: DFTracer format - see https://dftracer.readthedocs.io/en/latest/trace_format.html

For DataCrumbs-specific Jupyter notebook examples, refer to the ``analysis/`` directory in the repository.

Advanced Configuration
======================

Trace and Log Directory Configuration
--------------------------------------

**Trace Directory** (Administrator Only):

The trace directory is configured by the system administrator during installation and **cannot be changed by users at runtime**.

.. code-block:: bash

    # Administrator sets trace directory at build time:
    # cmake -DDATACRUMBS_CONFIGURED_TRACE_DIR=/scratch/traces ..
    # 
    # Or via project YAML configuration:
    # trace_dir: /lustre/traces
    # trace_dir_pattern: /scratch/traces/%YY%/%MM%/%DD%

**Log Directory**:

Log directory may be configurable depending on administrator settings:

.. code-block:: bash

    # If enabled by administrator, users can set log directory
    export DATACRUMBS_CONFIGURED_LOG_DIR=/scratch/logs
    datacrumbs_run --app-name myapp -- ./myapp

Filtering Traced Paths
-----------------------

**Administrator Configuration Only**

Path filtering must be configured at build time by the system administrator:

.. code-block:: bash

    # Administrator configures path filtering at build time
    cmake -DDATACRUMBS_INCLUSION_PATH=/scratch/data ..
    make install

    # After installation, only I/O to /scratch/data will be traced
    datacrumbs_run --app-name filtered-app -- ./myapp

.. note::
   Users cannot change the inclusion path at runtime. This must be set during installation.

Troubleshooting
===============

Permission Issues
-----------------

If you encounter permission errors:

.. code-block:: bash

    # Ensure eBPF capabilities (datacrumbs binary is in sbin)
    sudo setcap cap_sys_admin,cap_bpf,cap_perfmon+ep $PREFIX/sbin/datacrumbs

    # Or run with sudo
    sudo -E datacrumbs_run --app-name myapp -- ./myapp

Missing Traces
--------------

If traces are not generated:

.. code-block:: bash

    # Check if DataCrumbs is running
    ps aux | grep datacrumbs

    # Check logs (default is /tmp unless configured otherwise)
    cat /tmp/datacrumbs*.log
    # Or if you know your configured log directory:
    # cat ${DATACRUMBS_CONFIGURED_LOG_DIR}/datacrumbs*.log

    # Verify trace directory permissions (default is /tmp)
    ls -ld /tmp/*trace* 2>/dev/null || echo "No trace files found"

Performance Impact
------------------

To reduce tracing overhead:

.. code-block:: bash

    # Use profiling mode instead of full tracing
    cmake -DDATACRUMBS_MODE_STR=PROFILE ..
    make install

    # Skip small events
    cmake -DDATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS=10000 ..
    make install

For more information on running DataCrumbs in production environments, see the :doc:`usage` section.