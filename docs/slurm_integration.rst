============================
SLURM Scheduler Integration
============================

DataCrumbs provides deep integration with the SLURM workload manager for automated tracing in HPC environments. This integration enables transparent tracing of batch jobs through SLURM prolog/epilog scripts, systemd service management, and a custom ``salloc`` wrapper.

Overview
========

The SLURM integration consists of four components:

1. **Systemd Service**: Manages DataCrumbs server lifecycle
2. **Prolog Script**: Starts DataCrumbs before job execution
3. **Epilog Script**: Stops DataCrumbs after job completion
4. **salloc Wrapper**: Provides command-line options for users via ``datacrumbs_salloc``

Benefits
--------

* **Automatic Tracing**: Jobs are traced transparently without manual server management
* **Per-Job Isolation**: Each job gets its own tracing session
* **Custom Configurations**: Users can specify composable configurations
* **No Code Changes**: Applications remain unmodified
* **Centralized Management**: Administrators control tracing infrastructure
* **Native SLURM Integration**: Works seamlessly with existing SLURM workflows

Architecture
============

When a SLURM job is submitted with ``datacrumbs_salloc --datacrumbs-enable <REGULAR SLURM OPTIONS>``, the following sequence occurs:

1. **Job Submission Phase**:

   - User runs ``datacrumbs_salloc`` wrapper instead of ``salloc``
   - Wrapper parses DataCrumbs-specific flags
   - Job metadata is encoded as JSON in SLURM comment field
   - Allocation is submitted to SLURM with embedded DataCrumbs metadata

2. **Prolog Phase** (before job starts):

   - ``datacrumbs_service_run.sh`` is executed on compute node
   - Script parses SLURM job comment to check if tracing is enabled
   - Systemd service ``datacrumbs@<jobid>`` is created and started
   - DataCrumbs server begins collecting events
   - Job waits for server to be ready

3. **Execution Phase**:

   - Application runs normally with tracing active
   - eBPF probes capture I/O events
   - Events are written to trace files

4. **Epilog Phase** (after job completes):

   - ``datacrumbs_service_stop.sh`` is executed
   - Systemd service is stopped and disabled
   - Server flushes remaining events
   - Trace files are finalized

Installation Steps
==================

.. note::
   Installation requires root/administrator privileges on all compute nodes.

Step 1: Enable SLURM Prolog/Epilog
-----------------------------------

Edit the SLURM configuration file on the controller node:

.. code-block:: bash

    # On SLURM controller node
    sudo vi /etc/slurm/slurm.conf

Add or ensure the following configuration:

.. code-block:: text

    # Enable prolog execution
    Prolog=/etc/slurm/prolog.sh
    
    # Enable epilog execution
    Epilog=/etc/slurm/epilog.sh

.. note::
   Some SLURM configurations may use ``PrologSlurmctld`` and ``EpilogSlurmctld`` for controller-based execution. Adjust based on your site configuration. The prolog.sh and epilog.sh wrapper scripts will call all scripts in their respective .d directories.

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

    # Example for compute nodes compute001-compute128
    NODES="compute{001..128}"

    for node in $NODES; do
        echo "Installing on $node"
        ssh $node "sudo ln -sf ${DATACRUMBS_INSTALL}/etc/datacrumbs/systemd/datacrumbs@.service /etc/systemd/system/ && sudo systemctl daemon-reload"
    done

Step 3: Create Prolog Wrapper Script
-------------------------------------

Create a wrapper script that SLURM will call, which in turn executes all scripts in the prolog.d directory:

.. code-block:: bash

    # On each compute node
    sudo tee /etc/slurm/prolog.sh > /dev/null << 'EOF'
    #!/bin/bash
    # SLURM Prolog wrapper - executes all scripts in prolog.d
    
    PROLOG_DIR="/etc/slurm/prolog.d"
    
    if [ -d "$PROLOG_DIR" ]; then
        for script in "$PROLOG_DIR"/*; do
            if [ -x "$script" ]; then
                "$script"
            fi
        done
    fi
    EOF
    
    # Make executable
    sudo chmod +x /etc/slurm/prolog.sh

**Automated deployment:**

.. code-block:: bash

    NODES="compute{001..128}"

    for node in $NODES; do
        echo "Creating prolog wrapper on $node"
        ssh $node 'sudo tee /etc/slurm/prolog.sh > /dev/null << '"'"'EOF'"'"'
    #!/bin/bash
    PROLOG_DIR="/etc/slurm/prolog.d"
    if [ -d "$PROLOG_DIR" ]; then
        for script in "$PROLOG_DIR"/*; do
            if [ -x "$script" ]; then
                "$script"
            fi
        done
    fi
    EOF
    sudo chmod +x /etc/slurm/prolog.sh'
    done

Step 4: Install DataCrumbs Prolog Script
-----------------------------------------

Install the DataCrumbs prolog script that starts tracing:

**Script location:** ``<install-prefix>/sbin/datacrumbs_service_run.sh``

.. code-block:: bash

    # On each compute node
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install

    # Create prolog.d directory if it doesn't exist
    sudo mkdir -p /etc/slurm/prolog.d

    # Symlink DataCrumbs service run script
    sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh \\
        /etc/slurm/prolog.d/datacrumbs_service_run.sh

**Automated deployment:**

.. code-block:: bash

    NODES="compute{001..128}"

    for node in $NODES; do
        echo "Installing DataCrumbs prolog on $node"
        ssh $node "sudo mkdir -p /etc/slurm/prolog.d && sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh /etc/slurm/prolog.d/datacrumbs_service_run.sh"
    done

Step 5: Create Epilog Wrapper Script
-------------------------------------

Create a wrapper script that SLURM will call, which in turn executes all scripts in the epilog.d directory:

.. code-block:: bash

    # On each compute node
    sudo tee /etc/slurm/epilog.sh > /dev/null << 'EOF'
    #!/bin/bash
    # SLURM Epilog wrapper - executes all scripts in epilog.d
    
    EPILOG_DIR="/etc/slurm/epilog.d"
    
    if [ -d "$EPILOG_DIR" ]; then
        for script in "$EPILOG_DIR"/*; do
            if [ -x "$script" ]; then
                "$script"
            fi
        done
    fi
    EOF
    
    # Make executable
    sudo chmod +x /etc/slurm/epilog.sh

**Automated deployment:**

.. code-block:: bash

    NODES="compute{001..128}"

    for node in $NODES; do
        echo "Creating epilog wrapper on $node"
        ssh $node 'sudo tee /etc/slurm/epilog.sh > /dev/null << '"'"'EOF'"'"'
    #!/bin/bash
    EPILOG_DIR="/etc/slurm/epilog.d"
    if [ -d "$EPILOG_DIR" ]; then
        for script in "$EPILOG_DIR"/*; do
            if [ -x "$script" ]; then
                "$script"
            fi
        done
    fi
    EOF
    sudo chmod +x /etc/slurm/epilog.sh'
    done

Step 6: Install DataCrumbs Epilog Script
-----------------------------------------

Install the DataCrumbs epilog script that stops tracing:

**Script location:** ``<install-prefix>/sbin/datacrumbs_service_stop.sh``

.. code-block:: bash

    # On each compute node
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install

    # Create epilog.d directory if it doesn't exist
    sudo mkdir -p /etc/slurm/epilog.d

    # Symlink DataCrumbs service stop script
    sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh \\
        /etc/slurm/epilog.d/datacrumbs_service_stop.sh

**Automated deployment:**

.. code-block:: bash

    NODES="compute{001..128}"

    for node in $NODES; do
        echo "Installing DataCrumbs epilog on $node"
        ssh $node "sudo mkdir -p /etc/slurm/epilog.d && sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh /etc/slurm/epilog.d/datacrumbs_service_stop.sh"
    done

Step 7: Install datacrumbs_salloc Wrapper
------------------------------------------

The ``datacrumbs_salloc`` wrapper is installed as part of the standard DataCrumbs installation:

**Wrapper location:** ``<install-prefix>/bin/datacrumbs_salloc``

Users should add the DataCrumbs bin directory to their PATH:

.. code-block:: bash

    # Add to user's ~/.bashrc or ~/.bash_profile
    export PATH=/path/to/datacrumbs/install/bin:$PATH

**Verify wrapper installation:**

.. code-block:: bash

    # Check if wrapper is accessible
    which datacrumbs_salloc

    # View wrapper help
    datacrumbs_salloc --help

Step 8: Restart SLURM (if needed)
----------------------------------

After modifying ``slurm.conf``, restart SLURM services:

.. code-block:: bash

    # On controller node
    sudo systemctl restart slurmctld

    # On compute nodes (if using slurmd)
    sudo systemctl restart slurmd

If using ``scontrol``, you can reconfigure without full restart:

.. code-block:: bash

    sudo scontrol reconfigure

Complete Installation Script
=============================

Here's a complete script for deploying across multiple nodes:

.. code-block:: bash

    #!/bin/bash
    # deploy_datacrumbs_slurm.sh

    # Configuration
    DATACRUMBS_INSTALL=/path/to/datacrumbs/install
    NODES="compute{001..128}"

    echo "Deploying DataCrumbs SLURM integration..."

    # Deploy to each node
    for node in $(eval echo $NODES); do
        echo "=========================================="
        echo "Deploying to $node"
        echo "=========================================="

        ssh $node << EOF
    # Install systemd service
    sudo ln -sf ${DATACRUMBS_INSTALL}/etc/datacrumbs/systemd/datacrumbs@.service \\
        /etc/systemd/system/datacrumbs@.service
    sudo systemctl daemon-reload

    # Create SLURM directories
    sudo mkdir -p /etc/slurm/prolog.d
    sudo mkdir -p /etc/slurm/epilog.d

    # Create prolog wrapper script
    sudo tee /etc/slurm/prolog.sh > /dev/null << 'PROLOG_EOF'
#!/bin/bash
PROLOG_DIR="/etc/slurm/prolog.d"
if [ -d "\$PROLOG_DIR" ]; then
    for script in "\$PROLOG_DIR"/*; do
        if [ -x "\$script" ]; then
            "\$script"
        fi
    done
fi
PROLOG_EOF
    sudo chmod +x /etc/slurm/prolog.sh

    # Create epilog wrapper script
    sudo tee /etc/slurm/epilog.sh > /dev/null << 'EPILOG_EOF'
#!/bin/bash
EPILOG_DIR="/etc/slurm/epilog.d"
if [ -d "\$EPILOG_DIR" ]; then
    for script in "\$EPILOG_DIR"/*; do
        if [ -x "\$script" ]; then
            "\$script"
        fi
    done
fi
EPILOG_EOF
    sudo chmod +x /etc/slurm/epilog.sh

    # Symlink DataCrumbs prolog script
    sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh \\
        /etc/slurm/prolog.d/datacrumbs_service_run.sh

    # Symlink DataCrumbs epilog script
    sudo ln -sf ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh \\
        /etc/slurm/epilog.d/datacrumbs_service_stop.sh

    # Clean old runtime files
    sudo rm -rf /tmp/datacrumbs*
    sudo rm -rf /var/run/datacrumbs*

    echo "✓ Installation complete on $node"
    EOF
    done

    echo ""
    echo "=========================================="
    echo "Deployment complete!"
    echo "=========================================="
    echo ""
    echo "Next steps:"
    echo "1. Verify slurm.conf has Prolog=/etc/slurm/prolog.sh and Epilog=/etc/slurm/epilog.sh"
    echo "2. Run: sudo scontrol reconfigure (on controller)"
    echo "3. Add DataCrumbs bin to user PATH"
    echo "4. Test: datacrumbs_salloc --datacrumbs-enable -N 1"

Usage
=====

Basic Usage
-----------

.. important::
   Before tracing, applications must be instrumented using ``datacrumbs_track`` or ``datacrumbs_wrap``. DataCrumbs cannot trace uninstrumented executables. See the main documentation for tracking instructions.

Use ``datacrumbs_salloc`` wrapper instead of ``salloc`` to enable DataCrumbs:

.. code-block:: bash

    datacrumbs_salloc --datacrumbs-enable [SLURM_OPTIONS]

Example:

.. code-block:: bash

    # Single node allocation
    datacrumbs_salloc --datacrumbs-enable -N 1

    # Multi-node allocation
    datacrumbs_salloc --datacrumbs-enable -N 4 -n 32

    # With time limit and partition
    datacrumbs_salloc --datacrumbs-enable -N 2 -t 1:00:00 -p compute

With Custom Composable Configuration
-------------------------------------

Specify a custom composable configuration:

.. code-block:: bash

    datacrumbs_salloc --datacrumbs-enable --datacrumbs-composite=CONFIG_NAME [SLURM_OPTIONS]

Example:

.. code-block:: bash

    # Use POSIX I/O configuration
    datacrumbs_salloc --datacrumbs-enable --datacrumbs-composite=posix_io -N 4

    # Use HDF5 configuration
    datacrumbs_salloc --datacrumbs-enable --datacrumbs-composite=hdf5_only -N 2

    # Use MPI-IO configuration
    datacrumbs_salloc --datacrumbs-enable --datacrumbs-composite=mpiio_only -N 8

Running Jobs in Allocation
---------------------------

Once you have an allocation with DataCrumbs enabled, run your applications normally:

.. code-block:: bash

    # Get allocation with tracing
    datacrumbs_salloc --datacrumbs-enable -N 4

    # Inside allocation, run jobs
    srun -N 4 -n 32 ./myapp

    # MPI application
    srun -N 4 -n 64 ./parallel_simulation

    # Exit allocation when done
    exit

Using with sbatch
-----------------

For batch jobs, you need to set the job comment manually in your batch script:

.. code-block:: bash

    #!/bin/bash
    #SBATCH -N 4
    #SBATCH -n 32
    #SBATCH -t 1:00:00
    #SBATCH --comment='{"datacrumbs": {"enable": "yes", "composite": "posix_io"}}'

    # Your application
    srun ./myapp

.. note::
   The ``--comment`` flag must contain valid JSON with the DataCrumbs metadata. The wrapper handles this automatically for interactive allocations.

Command Reference
=================

datacrumbs_salloc Options
--------------------------

``--datacrumbs-enable``
    Enable DataCrumbs tracing for the allocation

    - Default: disabled
    - No argument required
    - Must be specified to activate tracing

``--datacrumbs-composite=NAME``
    Specify custom composable configuration

    - Requires ``--datacrumbs-enable``
    - NAME must be alphanumeric and underscores only
    - Composable must exist (created with ``datacrumbs_compose``)
    - If not specified, uses default configuration

.. warning::
   The ``--comment`` flag is reserved for internal use by the wrapper and cannot be used directly.

Examples
========

Example 1: Interactive Job with Tracing
----------------------------------------

.. code-block:: bash

    # Get allocation with tracing
    datacrumbs_salloc --datacrumbs-enable -N 1

    # Run I/O benchmark
    srun dd if=/dev/zero of=/tmp/test bs=1M count=100

    # Check traces (will be available after epilog)
    exit

Example 2: Multi-Node MPI Application
--------------------------------------

.. code-block:: bash

    # Get allocation with tracing
    datacrumbs_salloc --datacrumbs-enable -N 8 -n 64

    # Run MPI application
    srun -N 8 -n 64 ./parallel_simulation input.dat

    # Exit when done
    exit

Example 3: Custom Configuration for HDF5
-----------------------------------------

.. code-block:: bash

    # First, create HDF5-specific configuration (one-time, as root/admin)
    sudo datacrumbs_compose --action compose \\
        --name hdf5_workflow \\
        --probes H5Fopen,H5Fclose,H5Dcreate,H5Dopen,H5Dclose,H5Dread,H5Dwrite

    # Get allocation with custom config
    datacrumbs_salloc --datacrumbs-enable --datacrumbs-composite=hdf5_workflow -N 4

    # Run HDF5 application
    srun -N 4 -n 32 ./hdf5_analysis dataset.h5

    # Exit
    exit

Example 4: Batch Job with Tracing
----------------------------------

Create a batch script ``job.sbatch``:

.. code-block:: bash

    #!/bin/bash
    #SBATCH -J datacrumbs_test
    #SBATCH -N 4
    #SBATCH -n 32
    #SBATCH -t 1:00:00
    #SBATCH -p compute
    #SBATCH --comment='{"datacrumbs": {"enable": "yes", "composite": "NONE"}}'

    module load ior

    # Run IOR benchmark
    srun -N 4 -n 32 ior -t 1m -b 16m -F -o /scratch/testfile

Submit the job:

.. code-block:: bash

    sbatch job.sbatch

Trace File Location
===================

Traces are saved to the configured trace directory in **DFTracer format** (``.pfw.gz``).

**Trace Directory Pattern**:

According to the ``trace_dir_pattern`` in the project configuration:

.. code-block:: bash

    # Default pattern: /path/to/traces/%YY%/%MM%/%DD%
    # For job on 2025-12-03:
    /path/to/traces/25/12/03/

**Trace File Naming**:

.. code-block:: text

    # Without MPI:
    trace-<user>-<jobid>-<hostname>-<config>.pfw.gz
    
    # With MPI:
    trace-<user>-<jobid>-<rank>-<size>-<config>.pfw.gz

**Example**:

.. code-block:: bash

    # Single node job (SLURM_JOB_ID=12345)
    /path/to/traces/25/12/03/trace-haridev-12345-node01-lead.pfw.gz
    
    # Multi-rank MPI job (4 ranks, SLURM_JOB_ID=67890)
    /path/to/traces/25/12/03/trace-haridev-67890-0-4-lead.pfw.gz
    /path/to/traces/25/12/03/trace-haridev-67890-1-4-lead.pfw.gz
    /path/to/traces/25/12/03/trace-haridev-67890-2-4-lead.pfw.gz
    /path/to/traces/25/12/03/trace-haridev-67890-3-4-lead.pfw.gz

Find traces for a specific job:

.. code-block:: bash

    # Set job ID
    JOBID=12345

    # Find trace files
    find $DATACRUMBS_TRACE_DIR -name "trace-*-${JOBID}-*.pfw.gz"
    
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

    # View SLURM logs
    sudo tail -f /var/log/slurm/slurmctld.log

    # Check job info
    scontrol show job <jobid>

    # View job comment (contains DataCrumbs metadata)
    scontrol show job <jobid> | grep Comment

Check Job Comment
-----------------

.. code-block:: bash

    # View DataCrumbs metadata in job comment
    JOBID=<your-job-id>
    scontrol show job $JOBID | grep Comment | awk -F'=' '{print $2}' | jq .

Debug Mode
----------

Enable verbose output in prolog/epilog scripts:

.. code-block:: bash

    # Edit prolog script
    sudo vi /etc/slurm/prolog.d/datacrumbs_service_run.sh

    # Add at the beginning:
    export DATACRUMBS_VERBOSE=1

Troubleshooting
===============

Job Hangs at Prolog
--------------------

**Symptom**: Job hangs during prolog execution

**Cause**: DataCrumbs server failed to start

**Solution**:

.. code-block:: bash

    # Check systemd service
    sudo journalctl -u "datacrumbs@*" --no-pager

    # Check for permission issues
    ls -la /var/run/datacrumbs/

    # Verify server binary exists
    ls -la $DATACRUMBS_INSTALL/sbin/datacrumbs

    # Check prolog script execution
    sudo tail -f /var/log/slurm/slurmctld.log

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

    # Verify job comment was set correctly
    scontrol show job <jobid> | grep Comment
    
    # Should show: Comment={"datacrumbs": {"enable": "yes", ...}}

    # Check if trace files exist
    ls -lh $DATACRUMBS_TRACE_DIR/trace-*.pfw.gz
    
    # Check trace directory permissions
    ls -la $DATACRUMBS_TRACE_DIR

    # Verify application was instrumented
    # Applications MUST be tracked or wrapped before tracing works
    ldd ./myapp | grep datacrumbs
    
    # Check service logs
    sudo journalctl -u "datacrumbs@*" --since "1 hour ago"

    # Verify eBPF support
    ls /sys/kernel/btf/vmlinux

Wrapper Not Found
-----------------

**Symptom**: ``datacrumbs_salloc: command not found``

**Cause**: DataCrumbs bin directory not in PATH

**Solution**:

.. code-block:: bash

    # Add to PATH temporarily
    export PATH=/path/to/datacrumbs/install/bin:$PATH

    # Add to ~/.bashrc for persistence
    echo 'export PATH=/path/to/datacrumbs/install/bin:$PATH' >> ~/.bashrc
    source ~/.bashrc

Invalid JSON in Comment
-----------------------

**Symptom**: Job fails with JSON parsing error

**Cause**: Malformed JSON in ``--comment`` field (when using sbatch)

**Solution**:

.. code-block:: bash

    # Validate JSON format
    echo '{"datacrumbs": {"enable": "yes", "composite": "NONE"}}' | jq .

    # Use single quotes around comment in sbatch script
    #SBATCH --comment='{"datacrumbs": {"enable": "yes", "composite": "NONE"}}'

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
    sudo pkill -f "datacrumbs.*${JOBID}"

    # Clean up runtime files
    sudo rm -rf /var/run/datacrumbs/*${JOBID}*

Best Practices
==============

1. **Test First**: Test integration on a single node before cluster-wide deployment
2. **Monitor Overhead**: Check that tracing overhead is acceptable for your workloads
3. **Use Composables**: Create workload-specific configurations to minimize overhead
4. **Regular Cleanup**: Periodically clean old trace files to manage disk usage
5. **Log Rotation**: Configure log rotation for DataCrumbs and systemd logs
6. **Resource Limits**: Set appropriate ulimits in systemd service file
7. **Documentation**: Document custom composables and their use cases
8. **User Training**: Educate users on when and how to use ``datacrumbs_salloc``
9. **Disk Space**: Ensure adequate space for trace files
10. **Backup Configs**: Keep backups of configuration files and composables
11. **PATH Setup**: Document PATH requirements for users in site documentation
12. **JSON Validation**: Validate JSON when manually setting comments in batch scripts

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
    if [[ ! " user1 user2 user3 " =~ " $SLURM_JOB_USER " ]]; then
        exit 0  # Skip tracing
    fi

See Also
========

* :doc:`composability` - Creating custom composable configurations
* :doc:`deployment` - General deployment guidelines
* :doc:`usage` - DataCrumbs usage and configuration

For additional support or questions about SLURM integration, consult your system administrator or refer to the DataCrumbs GitHub repository.
