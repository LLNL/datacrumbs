============================
Flux Scheduler Integration
============================

DataCrumbs provides deep integration with the Flux resource manager for automated tracing in HPC environments. This integration enables transparent tracing of batch jobs through Flux job manager prologs/epilogs and systemd service management.

Overview
========

The Flux integration consists of three components:

1. **Systemd Service**: Manages DataCrumbs server lifecycle
2. **Prolog Script**: Starts DataCrumbs before job execution
3. **Epilog Script**: Stops DataCrumbs after job completion
4. **Flux Plugin**: Provides command-line options for users

Benefits
--------

* **Automatic Tracing**: Jobs are traced transparently without manual server management
* **Per-Job Isolation**: Each job gets its own tracing session
* **Custom Configurations**: Users can specify composable configurations
* **No Code Changes**: Applications remain unmodified
* **Centralized Management**: Administrators control tracing infrastructure

Architecture
============

When a Flux job is submitted with ``--datacrumbs-enable``:

1. **Prolog Phase** (before job starts):

   - ``datacrumbs_service_run.sh`` is executed
   - Systemd service ``datacrumbs@<jobid>`` is created and started
   - DataCrumbs server begins collecting events
   - Job waits for server to be ready

2. **Execution Phase**:

   - Application runs normally with tracing active
   - eBPF probes capture I/O events
   - Events are written to trace files

3. **Epilog Phase** (after job completes):

   - ``datacrumbs_service_stop.sh`` is executed
   - Systemd service is stopped and disabled
   - Server flushes remaining events
   - Trace files are finalized

Installation Steps
==================

.. note::
   Installation requires root/administrator privileges on all compute nodes.

Step 1: Enable Flux Prolog/Epilog
----------------------------------

Edit the Flux job manager configuration file on the broker node:

.. code-block:: bash

    # On Flux broker-0 node
    sudo vi /etc/flux/system/conf.d/job-manager.toml

Add or ensure the following configuration:

.. code-block:: toml

    [job-manager]

    # Enable prolog execution
    prolog.command = [
        "/etc/flux/system/prolog-job-manager.d/*",
    ]

    # Enable epilog execution
    epilog.command = [
        "/etc/flux/system/epilog.d/*",
    ]

Step 2: Install Systemd Service
--------------------------------

Install the DataCrumbs systemd service file on all compute nodes:

**Service file location:** ``<install-prefix>/etc/datacrumbs/systemd/datacrumbs@.service``

.. code-block:: bash

    # On each compute node
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install

    # Create symlink to systemd directory
    sudo ln -s ${DATACRUMBS_INSTALL}/etc/datacrumbs/systemd/datacrumbs@.service \\
        /etc/systemd/system/datacrumbs@.service

    # Reload systemd
    sudo systemctl daemon-reload

    # Verify service is recognized
    systemctl status datacrumbs@test.service

**Automated deployment** across multiple nodes:

.. code-block:: bash

    # Example for nodes lead2-lead6, lead9-lead11
    NODES=\"lead{2..6} lead{9..11}\"

    for node in $NODES; do
        echo \"Installing on $node\"
        ssh $node \"sudo ln -sf ${DATACRUMBS_INSTALL}/etc/datacrumbs/systemd/datacrumbs@.service /etc/systemd/system/ && sudo systemctl daemon-reload\"
    done

Step 3: Install Prolog Script
------------------------------

Install the prolog script that starts DataCrumbs:

**Script location:** ``<install-prefix>/sbin/datacrumbs_service_run.sh``

.. code-block:: bash

    # On each compute node
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install

    # Create symlink to prolog directory
    sudo ln -s ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh \\
        /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh

    # Make executable
    sudo chmod u+x /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh

**Automated deployment:**

.. code-block:: bash

    NODES=\"lead{2..6} lead{9..11}\"

    for node in $NODES; do
        echo \"Installing prolog on $node\"
        ssh $node \"sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh /etc/flux/system/prolog-job-manager.d/ && sudo chmod u+x /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh\"
    done

Step 4: Install Epilog Script
------------------------------

Install the epilog script that stops DataCrumbs:

**Script location:** ``<install-prefix>/sbin/datacrumbs_service_stop.sh``

.. code-block:: bash

    # On each compute node
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install

    # Create symlink to epilog directory
    sudo ln -s ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh \\
        /etc/flux/system/epilog.d/datacrumbs_service_stop.sh

    # Make executable
    sudo chmod u+x /etc/flux/system/epilog.d/datacrumbs_service_stop.sh

**Automated deployment:**

.. code-block:: bash

    NODES=\"lead{2..6} lead{9..11}\"

    for node in $NODES; do
        echo \"Installing epilog on $node\"
        ssh $node \"sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh /etc/flux/system/epilog.d/ && sudo chmod u+x /etc/flux/system/epilog.d/datacrumbs_service_stop.sh\"
    done

Step 5: Install Flux Plugin
----------------------------

Install the Flux CLI plugin for DataCrumbs command-line options:

**Plugin location:** ``<install-prefix>/etc/datacrumbs/flux/cli/plugins/datacrumbs.py``

.. code-block:: bash

    # On Flux broker node (or all nodes with flux CLI)
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install

    # Create flux plugins directory if it doesn't exist
    sudo mkdir -p /etc/flux/cli/plugins

    # Copy plugin file
    sudo cp ${DATACRUMBS_INSTALL}/etc/datacrumbs/flux/cli/plugins/datacrumbs.py \\
        /etc/flux/cli/plugins/

    # Set permissions
    sudo chmod 644 /etc/flux/cli/plugins/datacrumbs.py

**Verify plugin installation:**

.. code-block:: bash

    # Check if plugin is loaded
    flux run --help | grep datacrumbs

    # Should show:
    #   --datacrumbs-enable    Enable datacrumbs
    #   --datacrumbs-composite COMPOSITE
    #                          Set composite name for datacrumbs

Step 6: Restart Flux (if needed)
---------------------------------

After modifying ``job-manager.toml``, reload Flux configuration:

.. code-block:: bash

    # On broker-0 node only
    flux config reload

If prolog/epilog entries were added for the first time, a full Flux restart may be required. Consult your system administrator.

Complete Installation Script
=============================

Here's a complete script for deploying across multiple nodes:

.. code-block:: bash

    #!/bin/bash
    # deploy_datacrumbs_flux.sh

    # Configuration
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install
    NODES=\"lead{2..6} lead{9..11}\"

    echo \"Deploying DataCrumbs Flux integration...\"

    # Deploy to each node
    for node in $(eval echo $NODES); do
        echo \"==========================================\"
        echo \"Deploying to $node\"
        echo \"==========================================\"

        ssh $node << EOF
    # Install systemd service
    sudo ln -sf ${DATACRUMBS_INSTALL}/etc/datacrumbs/systemd/datacrumbs@.service \\
        /etc/systemd/system/datacrumbs@.service
    sudo systemctl daemon-reload

    # Install prolog script
    sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh \\
        /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh
    sudo chmod u+x /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh

    # Install epilog script
    sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh \\
        /etc/flux/system/epilog.d/datacrumbs_service_stop.sh
    sudo chmod u+x /etc/flux/system/epilog.d/datacrumbs_service_stop.sh

    # Clean old runtime files
    sudo rm -rf /tmp/datacrumbs*
    sudo rm -rf /var/run/datacrumbs*

    echo \"✓ Installation complete on $node\"
    EOF
    done

    # Install Flux plugin on broker node
    echo \"==========================================\"
    echo \"Installing Flux plugin\"
    echo \"==========================================\"
    sudo mkdir -p /etc/flux/cli/plugins
    sudo cp ${DATACRUMBS_INSTALL}/etc/datacrumbs/flux/cli/plugins/datacrumbs.py \\
        /etc/flux/cli/plugins/
    sudo chmod 644 /etc/flux/cli/plugins/datacrumbs.py

    echo \"✓ Flux plugin installed\"
    echo \"\"
    echo \"==========================================\"
    echo \"Deployment complete!\"
    echo \"==========================================\"
    echo \"\"
    echo \"Next steps:\"
    echo \"1. Verify job-manager.toml has prolog/epilog configured\"
    echo \"2. Run: flux config reload (on broker-0)\"
    echo \"3. Test: flux run --datacrumbs-enable hostname\"

Usage
=====

Basic Usage
-----------

Enable DataCrumbs for a Flux job:

.. code-block:: bash

    flux run --datacrumbs-enable [OPTIONS] COMMAND

Example:

.. code-block:: bash

    # Single node job
    flux run --datacrumbs-enable hostname

    # Multi-node job
    flux run --datacrumbs-enable -N 4 -n 32 ./myapp

    # MPI application (Flux handles launch automatically)
    flux run --datacrumbs-enable -N 8 -n 64 ./parallel_app

With Custom Composable Configuration
-------------------------------------

Specify a custom composable configuration:

.. code-block:: bash

    flux run --datacrumbs-enable --datacrumbs-composite=CONFIG_NAME [OPTIONS] COMMAND

Example:

.. code-block:: bash

    # Use POSIX I/O configuration
    flux run --datacrumbs-enable --datacrumbs-composite=posix_io \\
        -N 4 -n 32 ./io_benchmark

    # Use HDF5 configuration
    flux run --datacrumbs-enable --datacrumbs-composite=hdf5_only \\
        -N 2 -n 16 ./hdf5_writer dataset.h5

    # Use MPI-IO configuration
    flux run --datacrumbs-enable --datacrumbs-composite=mpiio_only \\
        -N 8 -n 64 ./mpi_parallel_io

Command Reference
=================

Flux CLI Options
----------------

``--datacrumbs-enable``
    Enable DataCrumbs tracing for the job

    - Default: disabled
    - No argument required

``--datacrumbs-composite=NAME``
    Specify custom composable configuration

    - Requires ``--datacrumbs-enable``
    - NAME must be alphanumeric and underscores only
    - Composable must exist (created with ``datacrumbs_compose``)
    - If not specified, uses default full configuration

Examples
========

Example 1: Simple Job with Tracing
-----------------------------------

.. code-block:: bash

    # Submit job with tracing enabled
    flux run --datacrumbs-enable -n 1 dd if=/dev/zero of=/tmp/test bs=1M count=100

    # Check trace location (set in configuration)
    ls $DATACRUMBS_TRACE_DIR/

Example 2: Batch Job with Custom Configuration
-----------------------------------------------

.. code-block:: bash

    # Create custom configuration (one-time setup)
    sudo datacrumbs_compose --action compose \\
        --name io_benchmark \\
        --probes open,close,read,write,pread64,pwrite64

    # Submit batch job
    flux batch --datacrumbs-enable --datacrumbs-composite=io_benchmark <<EOF
    #!/bin/bash
    #SBATCH -N 4
    #SBATCH -n 32

    module load ior
    flux run -N 4 -n 32 ior -t 1m -b 16m -F -o /tmp/testfile
    EOF

Example 3: MPI Application
---------------------------

.. code-block:: bash

    # Submit MPI job with tracing
    # Flux handles the MPI launch automatically
    flux run --datacrumbs-enable \
        -N 8 -n 64 \
        --tasks-per-node 8 \
        ./parallel_simulation input.dat

Example 4: HDF5 Workflow
-------------------------

.. code-block:: bash

    # Create HDF5-specific configuration
    sudo datacrumbs_compose --action compose \\
        --name hdf5_workflow \\
        --probes H5Fopen,H5Fclose,H5Dcreate,H5Dopen,H5Dclose,H5Dread,H5Dwrite

    # Run workflow with custom configuration
    flux run --datacrumbs-enable --datacrumbs-composite=hdf5_workflow \\
        -N 4 -n 32 ./hdf5_analysis large_dataset.h5

Trace File Location
===================

Traces are saved to the configured trace directory in **DFTracer format** (``.pfw.gz``).

**Trace Directory Pattern**:

According to the ``trace_dir_pattern`` in the project configuration:

.. code-block:: bash

    # Default pattern: /path/to/traces/%YY%/%MM%/%DD%
    # For job on 2025-11-21:
    /path/to/traces/25/11/21/

**Trace File Naming**:

.. code-block:: text

    # Without MPI:
    trace-<user>-<run_id>-<hostname>-<config>.pfw.gz
    
    # With MPI:
    trace-<user>-<run_id>-<rank>-<size>-<config>.pfw.gz

**Example**:

.. code-block:: bash

    # Single node job
    /path/to/traces/25/11/21/trace-haridev-12345-node01-lead.pfw.gz
    
    # Multi-rank MPI job (4 ranks generate 4 trace files)
    /path/to/traces/25/11/21/trace-haridev-67890-0-4-lead.pfw.gz
    /path/to/traces/25/11/21/trace-haridev-67890-1-4-lead.pfw.gz
    /path/to/traces/25/11/21/trace-haridev-67890-2-4-lead.pfw.gz
    /path/to/traces/25/11/21/trace-haridev-67890-3-4-lead.pfw.gz

Find traces for a specific job:

.. code-block:: bash

    # Get job ID
    JOBID=$(flux jobs --format=\"{id}\" | head -1)

    # Find trace files
    find $DATACRUMBS_TRACE_DIR -name \"trace-*-${JOBID}-*.pfw.gz\"
    
    # List all traces from today
    ls -lh $DATACRUMBS_TRACE_DIR/$(date +%y/%m/%d)/trace-*.pfw.gz

Monitoring and Debugging
=========================

Check Service Status
--------------------

.. code-block:: bash

    # Check if service is running for a job
    JOBID=<your-job-id>
    systemctl status datacrumbs@${JOBID}.service

    # View service logs
    sudo journalctl -u datacrumbs@${JOBID}.service -f

Check Prolog/Epilog Execution
------------------------------

.. code-block:: bash

    # View Flux logs for prolog/epilog
    flux dmesg | grep datacrumbs

    # View job exceptions
    flux job eventlog <jobid> | grep datacrumbs

Debug Mode
----------

Enable verbose output in prolog/epilog scripts:

.. code-block:: bash

    # Edit prolog script
    sudo vi /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh

    # Add at the beginning:
    export DATACRUMBS_VERBOSE=1

Troubleshooting
===============

Job Hangs at Prolog
--------------------

**Symptom**: Job hangs with message \"prolog running\"

**Cause**: DataCrumbs server failed to start

**Solution**:

.. code-block:: bash

    # Check systemd service
    sudo journalctl -u \"datacrumbs@*\" --no-pager

    # Check for permission issues
    ls -la /var/run/datacrumbs/

    # Verify server binary exists
    ls -la $DATACRUMBS_INSTALL/sbin/datacrumbs

Composable Not Found
--------------------

**Symptom**: Error about missing composable configuration

**Cause**: Specified composable doesn't exist

**Solution**:

.. code-block:: bash

    # List available composables
    ls $PREFIX/libexec/datacrumbs/sbin/$USER/

    # Create if missing
    sudo datacrumbs_compose --action compose --name <config> --probes <probes>

No Traces Generated
-------------------

**Symptom**: Job completes but no trace files (``.pfw.gz``) created

**Cause**: Multiple possible causes

**Solution**:

.. code-block:: bash

    # Check if trace files exist with correct naming
    ls -lh $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz
    
    # Check trace directory permissions
    ls -la $DATACRUMBS_TRACE_DIR

    # Verify application was instrumented
    # Applications MUST be tracked or wrapped before tracing works
    ldd ./myapp | grep datacrumbs
    
    # Check service logs
    sudo journalctl -u \"datacrumbs@*\" --since \"1 hour ago\"

    # Verify eBPF support
    ls /sys/kernel/btf/vmlinux

    # Check for event drops
    dmesg | grep -i bpf

Service Won't Stop
------------------

**Symptom**: Epilog fails, service remains running

**Cause**: Server process not responding

**Solution**:

.. code-block:: bash

    # Manually stop service
    JOBID=<job-id>
    sudo systemctl stop datacrumbs@${JOBID}.service

    # Force kill if needed
    sudo pkill -f \"datacrumbs.*${JOBID}\"

    # Clean up runtime files
    sudo rm -rf /var/run/datacrumbs/*${JOBID}*

Best Practices
==============

1. **Test First**: Test integration on a single node before cluster-wide deployment
2. **Monitor Overhead**: Check that tracing overhead is acceptable for your workloads
3. **Use Composables**: Create workload-specific configurations to minimize overhead
4. **Regular Cleanup**: Periodically clean old trace files
5. **Log Rotation**: Configure log rotation for DataCrumbs and systemd logs
6. **Resource Limits**: Set appropriate ulimits in systemd service file
7. **Documentation**: Document custom composables and their use cases
8. **User Training**: Educate users on when and how to use ``--datacrumbs-enable``
9. **Disk Space**: Ensure adequate space for trace files
10. **Backup Configs**: Keep backups of configuration files and composables

Advanced Configuration
======================

Customizing Systemd Service
----------------------------

Edit the systemd service file to adjust timeouts, resources, etc.:

.. code-block:: ini

    # /etc/systemd/system/datacrumbs@.service
    [Unit]
    Description=DataCrumbs I/O Tracer for Job %i

    [Service]
    Type=forking
    User=root
    EnvironmentFile=/var/run/datacrumbs/datacrumbs-%i.env
    ExecStart=/path/to/datacrumbs/sbin/datacrumbs ...
    TimeoutStartSec=120s
    TimeoutStopSec=60s
    LimitNOFILE=1048576
    LimitMEMLOCK=infinity

Per-User Trace Directories
---------------------------

Configure per-user trace directories in the project configuration:

.. code-block:: yaml

    # project.yaml
    trace:
      dir_pattern: /scratch/${USER}/traces/%YY%/%MM%/%DD%

Resource Quotas
---------------

Limit trace file size and count per user (filesystem-dependent):

.. code-block:: bash

    # Example with XFS quotas
    xfs_quota -x -c 'limit -u bsoft=100g bhard=120g $USER' /scratch

Conditional Tracing
-------------------

Enable tracing only for specific user groups or applications (modify prolog script):

.. code-block:: bash

    # In datacrumbs_service_run.sh
    # Only trace for specific users
    if [[ ! \" user1 user2 user3 \" =~ \" $FLUX_JOB_USER \" ]]; then
        exit 0  # Skip tracing
    fi
