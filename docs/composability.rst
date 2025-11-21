===================
Composability Guide
===================

DataCrumbs provides a powerful composability feature that allows users to create custom tracing configurations by selecting specific probes. This enables optimized tracing tailored to specific workload requirements, reducing overhead and improving performance.

Overview
========

During the build and installation process, DataCrumbs:

1. Discovers available probes in system libraries and kernel functions
2. Compiles each probe as a separate eBPF object file
3. Installs individual probe objects to ``<prefix>/libexec/datacrumbs/objects/``
4. Provides tools for discovering and composing custom configurations

This modular approach allows users to:

- Select only the probes relevant to their workload
- Reduce tracing overhead by excluding unnecessary probes
- Create multiple configurations for different applications
- Share configurations across users in multi-tenant environments

Probe Objects
=============

After installation, individual probe objects are available in:

.. code-block:: bash

    $PREFIX/libexec/datacrumbs/objects/

Example probe objects:

- ``open.o`` - open() system call
- ``read.o`` - read() system call
- ``write.o`` - write() system call
- ``H5Fopen.o`` - HDF5 file open
- ``MPI_File_open.o`` - MPI-IO file open
- ``pread64.o``, ``pwrite64.o`` - POSIX I/O functions
- And many more depending on your system configuration

Common objects (always required):

- ``common.o`` - Shared eBPF code
- ``init.bpf.o`` - Initialization code

Discovering Available Probes
=============================

Use ``datacrumbs_compose`` to discover all available probes:

**Command:**

.. code-block:: bash

    sudo datacrumbs_compose --action discover

**Output:**

.. code-block:: text

    [INFO 2025-11-21 21:19:50] After loading yaml for docker
    [INFO 2025-11-21 21:19:50] Discovering probe objects in /workspaces/datacrumbs/install/libexec/datacrumbs/objects...
    [INFO 2025-11-21 21:19:50] custom1
    [INFO 2025-11-21 21:19:50] sys
    [INFO 2025-11-21 21:19:50] libc
    [INFO 2025-11-21 21:19:50] os_page_cache
    [INFO 2025-11-21 21:19:50] python
    [INFO 2025-11-21 21:19:50] xfs
    [INFO 2025-11-21 21:19:50] iomap
    ...

The discover action lists all probes that can be composed into custom configurations.

Creating Composable Configurations
===================================

Use ``datacrumbs_compose`` to create a custom configuration:

**Command Syntax:**

.. code-block:: bash

    sudo datacrumbs_compose --action compose --name CONFIG_NAME --probes PROBE1,PROBE2,...

**Arguments:**

``--action compose``
    Create a new composable configuration

``--name NAME``
    Name for the custom configuration (alphanumeric and underscore only)

``--probes PROBES``
    Comma-separated list of probe names (no spaces)

Example: Local I/O Only
-----------------------

Create a configuration that traces only POSIX I/O operations:

.. code-block:: bash

    sudo datacrumbs_compose --action compose \
        --name local_io \
        --probes custom1,sys,os_page_cache,xfs,iomap

This creates: ``$PREFIX/libexec/datacrumbs/sbin/<user>/datacrumbs_local_io``

Example: HDF5 Only
------------------

Create a configuration for HDF5 applications:

.. code-block:: bash

    sudo datacrumbs_compose --action compose \
        --name hdf5_only \
        --probes h5a,h5d,h5all

Example: MPI-IO Only
--------------------

Create a configuration for MPI-IO tracing:

.. code-block:: bash

    sudo datacrumbs_compose --action compose \
        --name mpiio_only \
        --probes mpiio,sys

Example: Mixed Workload
-----------------------

Create a configuration for applications using POSIX and HDF5:

.. code-block:: bash

    sudo datacrumbs_compose --action compose \
        --name posix_hdf5 \
        --probes custom1,sys,hdf5

Using Composable Configurations
================================

Once created, composable configurations can be used with any DataCrumbs execution mode.

With datacrumbs_run
-------------------

.. code-block:: bash

    # Use default configuration
    datacrumbs_run --app "./myapp"

    # Use custom configuration
    datacrumbs_compose_run --composite-name local_io --app "./myapp"

With Server Mode
----------------

.. code-block:: bash

    # Start server with custom configuration
    sudo datacrumbs_server_run.sh --composite-name hdf5_only

    # Run application (uses custom config)
    datacrumbs_track --executable ./hdf5_app
    ./hdf5_app

    # Stop server
    sudo datacrumbs_server_stop.sh --composite-name hdf5_only

With Service Mode
-----------------

.. code-block:: bash

    # Set environment variable for service
    export DATACRUMBS_COMPOSABLE_NAME=mpiio_only

    # Start service
    sudo datacrumbs_service_run.sh

    # Applications will use the mpiio_only configuration

With Flux Integration
---------------------

.. code-block:: bash

    # Submit job with custom configuration
    flux run --datacrumbs-enable --datacrumbs-composite=posix_hdf5 \
        -N 4 -n 32 ./parallel_app

    # Or with default configuration
    flux run --datacrumbs-enable -N 4 -n 32 ./parallel_app

Command Reference: datacrumbs_compose
======================================

**Synopsis:**

.. code-block:: bash

    datacrumbs_compose --action ACTION [OPTIONS]

**Required Arguments:**

``--action ACTION``
    Action to perform:

    - ``discover``: List all available probes
    - ``compose``: Create a new composable configuration

**Optional Arguments:**

``--name NAME``
    Name for the composable configuration (required for ``compose`` action)

    - Must contain only alphanumeric characters and underscores
    - Will create binary: ``datacrumbs_<name>``

``--probes PROBES``
    Comma-separated list of probe names (required for ``compose`` action)

    - No spaces between probe names
    - Probe names must exist (use ``discover`` to list)

``--verbose``
    Enable verbose output

``--quiet``
    Suppress informational messages

``--dry-run``
    Show what would be done without executing

**Examples:**

.. code-block:: bash

    # Discover available probes
    sudo datacrumbs_compose --action discover

    # Create simple configuration
    sudo datacrumbs_compose --action compose \
        --name basic_io \
        --probes custom1,sys,os_page_cache

    # Create with verbose output
    sudo datacrumbs_compose --action compose \
        --name debug_config \
        --probes custom1,sys,os_page_cache \
        --verbose

    # Dry run to see what would happen
    sudo datacrumbs_compose --action compose \
        --name test_config \
        --probes custom1 \
        --dry-run

Composable Binary Locations
============================

Composable binaries are installed per-user:

.. code-block:: text

    <prefix>/libexec/datacrumbs/sbin/<username>/datacrumbs_<config_name>

Example:

.. code-block:: bash

    # For user 'alice' with config 'local_io'
    /opt/datacrumbs/libexec/datacrumbs/sbin/alice/datacrumbs_local_io

    # For user 'bob' with config 'hdf5_only'
    /opt/datacrumbs/libexec/datacrumbs/sbin/bob/datacrumbs_hdf5_only

This per-user structure allows:

- Multiple users to have different configurations
- Configuration isolation between users
- Shared installations with user-specific customizations

Managing Composables
====================

Listing Composables
-------------------

List all composable configurations for the current user:

.. code-block:: bash

    ls $PREFIX/libexec/datacrumbs/sbin/$USER/

Removing Composables
--------------------

Remove a composable configuration:

.. code-block:: bash

    rm $PREFIX/libexec/datacrumbs/sbin/$USER/datacrumbs_<config_name>

Example:

.. code-block:: bash

    rm $PREFIX/libexec/datacrumbs/sbin/$USER/datacrumbs_posix_io

Updating Composables
--------------------

To update a composable configuration, simply recreate it:

.. code-block:: bash

    # Recreate with updated probe list
    sudo datacrumbs_compose --action compose \
        --name local_io \
        --probes custom1,sys,os_page_cache,xfs,iomap

The old configuration is automatically replaced.

Composability Best Practices
=============================

1. **Start Small**: Begin with minimal probe sets and add more as needed
2. **Profile First**: Use the full configuration to identify which probes are relevant
3. **Workload-Specific**: Create different configurations for different application types
4. **Test Configurations**: Verify custom configurations work before production use
5. **Document Configs**: Maintain a list of configurations and their purposes
6. **Share Configs**: Coordinate configurations across team members for consistency
7. **Monitor Overhead**: Compare overhead between full and custom configurations
8. **Version Control**: Keep track of which probe combinations work best

Common Configuration Patterns
==============================

System Call Level
-----------------

Trace only low-level system calls:

.. code-block:: bash

    sudo datacrumbs_compose --action compose \
        --name syscalls_only \
        --probes custom1,sys

Local I/O Level
---------------

Trace local I/O operations:

.. code-block:: bash

    sudo datacrumbs_compose --action compose \
        --name local_io \
        --probes custom1,sys,os_page_cache,xfs,iomap

High-Level Libraries
--------------------

Trace only high-level I/O libraries:

.. code-block:: bash

    sudo datacrumbs_compose --action compose \
        --name high_level \
        --probes h5a,h5d,mpiio


Troubleshooting Composables
============================

Probe Not Found
---------------

.. code-block:: text

    Error: Probe object 'invalid_probe.o' not found.

**Solution**: Run ``discover`` to see available probes:

.. code-block:: bash

    sudo datacrumbs_compose --action discover

Invalid Configuration Name
--------------------------

.. code-block:: text

    Error: Invalid composable name

**Solution**: Use only alphanumeric characters and underscores:

.. code-block:: bash

    # Invalid
    sudo datacrumbs_compose --action compose --name my-config ...  # Hyphen not allowed

    # Valid
    sudo datacrumbs_compose --action compose --name my_config ...

Composable Not Used
-------------------

If the composable binary is created but not being used:

.. code-block:: bash

    # Verify binary exists
    ls -la $PREFIX/libexec/datacrumbs/sbin/$USER/datacrumbs_*

    # Check permissions
    # Should be: -r-x------ (500)

    # Verify you're specifying the name correctly
    datacrumbs_compose_run --composite-name exact_name

Build Failures
--------------

If composable build fails:

.. code-block:: bash

    # Check build logs in the dry-run output
    sudo datacrumbs_compose --action compose \
        --name test \
        --probes open,close \
        --verbose

    # Verify CMake and build tools are available
    which cmake

    # Check probe objects exist
    ls $PREFIX/libexec/datacrumbs/objects/

Advanced Usage
==============

Programmatic Composition
------------------------

Create multiple configurations programmatically:

.. code-block:: bash

    #!/bin/bash

    # Define configurations
    declare -A configs
    configs[posix]="open,close,read,write,lseek"
    configs[hdf5]="H5Fopen,H5Fclose,H5Dread,H5Dwrite"
    configs[mpiio]="MPI_File_open,MPI_File_close,MPI_File_read,MPI_File_write"

    # Create all configurations
    for name in "${!configs[@]}"; do
        echo "Creating configuration: $name"
        sudo datacrumbs_compose --action compose \
            --name "$name" \
            --probes "${configs[$name]}"
    done

Dynamic Probe Selection
-----------------------

Select probes based on application analysis:

.. code-block:: bash

    # Run with full tracing first
    datacrumbs_run --app "./myapp"

    # Analyze trace to find used functions
    # (Analysis script not shown)

    # Create optimized configuration with only used probes
    sudo datacrumbs_compose --action compose \
        --name myapp_optimized \
        --probes open,read,write,close,H5Dread,H5Dwrite

Integration with Workflows
---------------------------

Include composable creation in workflow scripts:

.. code-block:: bash

    #!/bin/bash
    # workflow.sh

    # Create workflow-specific configuration
    sudo datacrumbs_compose --action compose \
        --name workflow_io \
        --probes open,close,read,write,H5Fopen,H5Fclose

    # Run workflow stages with custom configuration
    datacrumbs_server_run --composite-name workflow_io
    datacrumbs_track --executable ./stage1_preprocess
    ./stage1_preprocess
    datacrumbs_track --executable ./stage2_compute
    ./stage2_compute
    datacrumbs_track --executable ./stage3_postprocess
    ./stage3_postprocess
    datacrumbs_server_stop --composite-name workflow_io

Examples
========

Example 1: Lightweight POSIX Tracing
-------------------------------------

.. code-block:: bash

    # Discover available probes
    sudo datacrumbs_compose --action discover

    # Create lightweight configuration
    sudo datacrumbs_compose --action compose \
        --name light_posix \
        --probes open,close,read,write

    # Use with application
    datacrumbs_compose_run --composite-name light_posix \
        --app "./io_benchmark"

    # Compare overhead with full tracing
    # Lightweight typically has 2-3% overhead vs 5-8% for full

Example 2: HDF5 Application Optimization
-----------------------------------------

.. code-block:: bash

    # Initial run with full tracing
    datacrumbs_run --app "./hdf5_app dataset.h5"

    # Analyze trace - shows only H5 functions are used

    # Create HDF5-only configuration
    sudo datacrumbs_compose --action compose \
        --name hdf5_optimized \
        --probes H5Fopen,H5Fclose,H5Fcreate,H5Dopen,H5Dclose,H5Dcreate,H5Dread,H5Dwrite

    # Use optimized configuration
    datacrumbs_compose_run --composite-name hdf5_optimized \
        --app "./hdf5_app large_dataset.h5"

Example 3: Multi-User Environment
----------------------------------

.. code-block:: bash

    # Administrator creates common configurations
    sudo su - admin

    # POSIX configuration
    sudo datacrumbs_compose --action compose \
        --name std_posix \
        --probes open,close,read,write,pread64,pwrite64

    # HDF5 configuration
    sudo datacrumbs_compose --action compose \
        --name std_hdf5 \
        --probes H5Fopen,H5Fclose,H5Dread,H5Dwrite

    # Users can now use these configurations
    # User 1
    datacrumbs_compose_run --composite-name std_posix --app "./user1_app"

    # User 2
    datacrumbs_compose_run --composite-name std_hdf5 --app "./user2_app"

Example 4: Flux Workflow
-------------------------

.. code-block:: bash

    # Create configuration for workflow
    sudo datacrumbs_compose --action compose \
        --name workflow_trace \
        --probes open,close,read,write,H5Dread,H5Dwrite,MPI_File_read,MPI_File_write

    # Submit Flux job with custom configuration
    flux run --datacrumbs-enable \
             --datacrumbs-composite=workflow_trace \
             -N 8 -n 64 \
             ./parallel_workflow
