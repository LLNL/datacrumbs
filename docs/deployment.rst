===================
Deployment for HPC
===================

DataCrumbs can be deployed on HPC systems in multiple ways to allow users to trace their applications. This guide covers deployment methods for multi-user environments where administrators need to grant users access to DataCrumbs functionality.

Deployment Overview
===================

There are two primary deployment approaches:

1. **Sudoers Access**: Grant specific users permission to start/stop the DataCrumbs server via sudo
2. **Scheduler Integration**: Use job scheduler prolog/epilog scripts to manage DataCrumbs automatically

.. list-table:: Deployment Method Comparison
   :header-rows: 1
   :widths: 25 35 40

   * - Method
     - Best For
     - Characteristics
   * - Sudoers Access
     - Interactive development, small teams
     - User-initiated, explicit control, manual start/stop
   * - Scheduler Integration
     - Production workloads, large deployments
     - Automatic, transparent, per-job isolation

Method 1: Sudoers File Access
==============================

This method grants specific users permission to run DataCrumbs server control scripts without requiring full root privileges.

Overview
--------

Users can:

- Start DataCrumbs server with ``sudo datacrumbs_server_run.sh``
- Stop DataCrumbs server with ``sudo datacrumbs_server_stop.sh``
- Cannot modify DataCrumbs installation or configuration
- Cannot access other users' trace data

Sudoers Configuration
---------------------

**Step 1: Create sudoers configuration file**

Create ``/etc/sudoers.d/datacrumbs`` with the following content:

.. code-block:: bash

    # Allow specific users to run DataCrumbs server control scripts

    # Define user aliases (customize for your site)
    User_Alias DATACRUMBS_USERS = user1, user2, user3, %hpc_group

    # Define command aliases
    Cmnd_Alias DATACRUMBS_START = /opt/datacrumbs/sbin/datacrumbs_server_run.sh
    Cmnd_Alias DATACRUMBS_STOP = /opt/datacrumbs/sbin/datacrumbs_server_stop.sh
    Cmnd_Alias DATACRUMBS_SERVICE_START = /opt/datacrumbs/sbin/datacrumbs_service_run.sh
    Cmnd_Alias DATACRUMBS_SERVICE_STOP = /opt/datacrumbs/sbin/datacrumbs_service_stop.sh

    # Grant permissions (no password required)
    DATACRUMBS_USERS ALL = (root) NOPASSWD: DATACRUMBS_START, DATACRUMBS_STOP
    DATACRUMBS_USERS ALL = (root) NOPASSWD: DATACRUMBS_SERVICE_START, DATACRUMBS_SERVICE_STOP

.. important::
   Replace ``/opt/datacrumbs`` with your actual DataCrumbs installation prefix.

**Step 2: Set correct permissions**

.. code-block:: bash

    # Set ownership and permissions
    sudo chown root:root /etc/sudoers.d/datacrumbs
    sudo chmod 0440 /etc/sudoers.d/datacrumbs

    # Validate sudoers syntax
    sudo visudo -c -f /etc/sudoers.d/datacrumbs

**Step 3: Verify configuration**

.. code-block:: bash

    # Test as regular user
    sudo -l

    # Expected output should include:
    # User <username> may run the following commands:
    #     (root) NOPASSWD: /opt/datacrumbs/sbin/datacrumbs_server_run.sh
    #     (root) NOPASSWD: /opt/datacrumbs/sbin/datacrumbs_server_stop.sh

User-Specific Configuration
----------------------------

**Allow per-user DataCrumbs access:**

.. code-block:: bash

    # In /etc/sudoers.d/datacrumbs
    User_Alias DATACRUMBS_ADMINS = admin1, admin2
    User_Alias DATACRUMBS_BASIC_USERS = user1, user2, %research_group

    # Admins can run all DataCrumbs commands
    DATACRUMBS_ADMINS ALL = (root) NOPASSWD: /opt/datacrumbs/sbin/*

    # Basic users can only start/stop their own servers
    DATACRUMBS_BASIC_USERS ALL = (root) NOPASSWD: DATACRUMBS_START, DATACRUMBS_STOP

Group-Based Access
------------------

Grant access to entire groups:

.. code-block:: bash

    # In /etc/sudoers.d/datacrumbs

    # Allow all users in 'hpc' group
    %hpc ALL = (root) NOPASSWD: DATACRUMBS_START, DATACRUMBS_STOP

    # Allow all users in multiple groups
    User_Alias DATACRUMBS_USERS = %hpc, %research, %developers

Usage by Regular Users
-----------------------

Once configured, users can start and stop DataCrumbs:

.. code-block:: bash

    # Start DataCrumbs server
    sudo datacrumbs_server_run.sh --user $USER

    # Stop DataCrumbs server
    sudo datacrumbs_server_stop.sh --user $USER

    # With service mode
    sudo datacrumbs_service_run.sh --user $USER
    sudo datacrumbs_service_stop.sh --user $USER

Security Considerations
-----------------------

.. warning::
   - Only grant access to trusted users
   - Regularly audit sudoers configuration
   - Monitor DataCrumbs server processes
   - Review trace logs for unauthorized access

Best practices:

1. Use specific command paths (not wildcards like ``/opt/datacrumbs/*``)
2. Require users to specify ``--user $USER`` to prevent impersonation
3. Set up log monitoring: ``/var/log/auth.log`` tracks sudo usage
4. Implement quota limits on trace storage directories
5. Rotate trace logs regularly

Restricting Commands
--------------------

Limit users to specific DataCrumbs configurations:

.. code-block:: bash

    # Only allow specific configurations
    Cmnd_Alias DATACRUMBS_SAFE_START = /opt/datacrumbs/sbin/datacrumbs_server_run.sh --user *, \
                                        --config standard

    DATACRUMBS_USERS ALL = (root) NOPASSWD: DATACRUMBS_SAFE_START

Method 2: Scheduler Integration
================================

Automatic DataCrumbs management via job scheduler prolog and epilog scripts. This is the recommended approach for production HPC environments.

Overview
--------

The scheduler automatically:

- Starts DataCrumbs server when job begins (prolog)
- Stops DataCrumbs server when job completes (epilog)
- Isolates tracing to specific jobs
- No user intervention required

Supported Schedulers
--------------------

* **Flux**: Full support with systemd integration
* **Slurm**: Supported via prolog/epilog scripts

Flux Integration
================

DataCrumbs integrates with Flux resource manager using systemd services and prolog/epilog scripts.

For complete Flux deployment instructions, see :doc:`flux_integration`.

Quick Flux Setup
----------------

**Step 1: Install systemd service**

.. code-block:: bash

    # Copy service file to all nodes
    for node in node{1..10}; do
        sudo scp $PREFIX/etc/datacrumbs/systemd/datacrumbs@.service \
                 $node:/etc/systemd/system/
    done

    # Reload systemd
    sudo systemctl daemon-reload

**Step 2: Install prolog/epilog scripts**

.. code-block:: bash

    # On all Flux nodes
    sudo ln -s $PREFIX/sbin/datacrumbs_service_run.sh \
               /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh

    sudo ln -s $PREFIX/sbin/datacrumbs_service_stop.sh \
               /etc/flux/system/epilog.d/datacrumbs_service_stop.sh

    # Make executable
    sudo chmod u+x /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh
    sudo chmod u+x /etc/flux/system/epilog.d/datacrumbs_service_stop.sh

**Step 3: Configure Flux job manager**

Edit ``/etc/flux/system/conf.d/job-manager.toml``:

.. code-block:: toml

    [job-manager]
    plugins = [
        { load = "prolog-job-manager.so" },
        { load = "epilog.so" }
    ]

**Step 4: Install Flux CLI Plugin**

The Flux CLI plugin can be installed system-wide or per-user.

**Option A: System-wide installation (recommended for HPC)**

.. code-block:: bash

    # Create flux plugins directory if it doesn't exist
    sudo mkdir -p /etc/flux/cli/plugins

    # Copy the DataCrumbs plugin
    sudo cp $PREFIX/etc/datacrumbs/flux/cli/plugins/datacrumbs.py \
        /etc/flux/cli/plugins/

    # Set permissions for all users
    sudo chmod 644 /etc/flux/cli/plugins/datacrumbs.py

**Option B: Per-user installation**

.. code-block:: bash

    # Create user plugin directory
    mkdir -p ~/plugins

    # Copy the DataCrumbs plugin
    cp $PREFIX/etc/datacrumbs/flux/cli/plugins/datacrumbs.py ~/plugins/

    # Set environment variable (add to ~/.bashrc for persistence)
    export FLUX_CLI_PLUGINPATH=~/plugins/

    # Use with flux commands
    flux run --datacrumbs-enable ./my-application

**Step 5: Restart Flux**

.. code-block:: bash

    # On broker-0 node
    sudo /admin/scripts/flux_reconfig.sh
    flux config reload

Multi-Node Deployment
---------------------

Deploy to all nodes in a cluster:

.. code-block:: bash

    #!/bin/bash
    # deploy_datacrumbs.sh

    NODES="node{1..10}"
    DATACRUMBS_INSTALL=/opt/datacrumbs

    for node in $(eval echo $NODES); do
        echo "Deploying to $node..."

        # Install systemd service
        sudo scp ${DATACRUMBS_INSTALL}/etc/datacrumbs/systemd/datacrumbs@.service \
                 ${node}:/etc/systemd/system/

        # Install prolog/epilog via symbolic links
        sudo ssh $node << 'EOF'
            # Remove old links
            rm -f /etc/flux/system/prolog-job-manager.d/datacrumbs*
            rm -f /etc/flux/system/epilog.d/datacrumbs*

            # Create new links
            ln -s ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh \
                  /etc/flux/system/prolog-job-manager.d/datacrumbs_service_run.sh
            ln -s ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh \
                  /etc/flux/system/epilog.d/datacrumbs_service_stop.sh

            # Set permissions
            chmod u+x /etc/flux/system/prolog-job-manager.d/datacrumbs*
            chmod u+x /etc/flux/system/epilog.d/datacrumbs*

            # Reload systemd
            systemctl daemon-reload
    EOF
    done

    # Install Flux CLI plugin on broker node (system-wide)
    echo "Installing Flux CLI plugin"
    sudo mkdir -p /etc/flux/cli/plugins
    sudo cp ${DATACRUMBS_INSTALL}/etc/datacrumbs/flux/cli/plugins/datacrumbs.py \
        /etc/flux/cli/plugins/
    sudo chmod 644 /etc/flux/cli/plugins/datacrumbs.py

    # Alternative: Install per-user (optional)
    # mkdir -p ~/plugins
    # cp ${DATACRUMBS_INSTALL}/etc/datacrumbs/flux/cli/plugins/datacrumbs.py ~/plugins/
    # echo "export FLUX_CLI_PLUGINPATH=~/plugins/" >> ~/.bashrc

User Workflow with Flux
------------------------

Users submit jobs normally with optional DataCrumbs flags:

.. code-block:: bash

    # Enable DataCrumbs for job
    flux run -N 4 --datacrumbs-enable ./my-application

    # With composable configuration
    flux run -N 4 \
             --datacrumbs-enable \
             --datacrumbs-composite=posix_only \
             ./my-application

If using per-user plugin installation:

.. code-block:: bash

    # Set plugin path (or add to ~/.bashrc)
    export FLUX_CLI_PLUGINPATH=~/plugins/

    # Then use flux commands normally
    flux run -N 4 --datacrumbs-enable ./my-application

No explicit DataCrumbs commands needed - prolog/epilog handle everything.



Slurm Integration
=================

DataCrumbs can be integrated with Slurm using similar prolog/epilog mechanisms.

Slurm Configuration
-------------------

**Step 1: Create prolog script**

Create ``/etc/slurm/prolog.d/datacrumbs.sh``:

.. code-block:: bash

    #!/bin/bash
    # /etc/slurm/prolog.d/datacrumbs.sh

    # Load DataCrumbs environment
    export DATACRUMBS_INSTALL=/opt/datacrumbs
    source ${DATACRUMBS_INSTALL}/bin/datacrumbs_setup

    # Start DataCrumbs server for job user
    if [[ "${SLURM_JOB_USER}" != "" ]]; then
        ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_run.sh \
            --user ${SLURM_JOB_USER} \
            --jobid ${SLURM_JOB_ID}
    fi

**Step 2: Create epilog script**

Create ``/etc/slurm/epilog.d/datacrumbs.sh``:

.. code-block:: bash

    #!/bin/bash
    # /etc/slurm/epilog.d/datacrumbs.sh

    export DATACRUMBS_INSTALL=/opt/datacrumbs
    source ${DATACRUMBS_INSTALL}/bin/datacrumbs_setup

    # Stop DataCrumbs server for job user
    if [[ "${SLURM_JOB_USER}" != "" ]]; then
        ${DATACRUMBS_INSTALL}/sbin/datacrumbs_service_stop.sh \
            --user ${SLURM_JOB_USER} \
            --jobid ${SLURM_JOB_ID}
    fi

**Step 3: Configure Slurm**

Edit ``/etc/slurm/slurm.conf``:

.. code-block:: ini

    # Enable prolog/epilog
    Prolog=/etc/slurm/prolog.d/datacrumbs.sh
    Epilog=/etc/slurm/epilog.d/datacrumbs.sh

    # Set timeouts
    PrologFlags=Alloc
    EpilogMsgTime=30

**Step 4: Set permissions and restart**

.. code-block:: bash

    # Set script permissions
    sudo chmod 755 /etc/slurm/prolog.d/datacrumbs.sh
    sudo chmod 755 /etc/slurm/epilog.d/datacrumbs.sh

    # Restart Slurm
    sudo systemctl restart slurmctld
    sudo systemctl restart slurmd

User Workflow with Slurm
-------------------------

Users submit jobs normally:

.. code-block:: bash

    # Submit batch job
    sbatch --nodes=4 my-job.sh

    # Interactive job
    salloc --nodes=4
    srun ./my-application

DataCrumbs automatically traces the application.

Verification and Testing
========================

After deployment, verify DataCrumbs is working correctly.

Test Sudoers Access
-------------------

.. code-block:: bash

    # As regular user
    sudo datacrumbs_server_run.sh --user $USER --verbose

    # Check server is running
    ps aux | grep datacrumbs

    # Stop server
    sudo datacrumbs_server_stop.sh --user $USER --verbose

Test Scheduler Integration
---------------------------

.. code-block:: bash

    # Submit simple test job (Flux)
    flux run -N 1 hostname

    # Check for DataCrumbs traces
    ls -la /tmp/datacrumbs_${USER}*/traces/

    # Submit with explicit enable (Slurm)
    sbatch test-job.sh

Monitor Deployment
------------------

Check systemd service status:

.. code-block:: bash

    # View service status
    sudo systemctl status datacrumbs@${USER}

    # View service logs
    sudo journalctl -u datacrumbs@${USER} -f

Check prolog/epilog execution:

.. code-block:: bash

    # Flux prolog/epilog logs
    sudo journalctl -u flux* --no-pager | grep datacrumbs

    # Slurm logs
    sudo tail -f /var/log/slurm/slurmctld.log | grep -i prolog

Troubleshooting Deployment
===========================

Common Issues
-------------

**"Permission denied" when starting server**

.. code-block:: bash

    # Check sudoers configuration
    sudo visudo -c -f /etc/sudoers.d/datacrumbs

    # Verify user is in allowed group
    groups $USER

**Prolog/epilog scripts not executing**

.. code-block:: bash

    # Flux: Check plugin configuration
    flux config get job-manager.plugins

    # Slurm: Check slurm.conf
    grep -i prolog /etc/slurm/slurm.conf

    # Verify script permissions
    ls -la /etc/flux/system/prolog-job-manager.d/
    ls -la /etc/slurm/prolog.d/

**Systemd service fails to start**

.. code-block:: bash

    # Check service status
    sudo systemctl status datacrumbs@${USER}

    # View detailed logs
    sudo journalctl -xe -u datacrumbs@${USER}

    # Verify service file
    sudo systemctl cat datacrumbs@.service

**Traces not being generated**

.. code-block:: bash

    # Check trace directory permissions
    ls -la /tmp/datacrumbs_${USER}*/

    # Verify DataCrumbs binary has correct capabilities
    getcap $PREFIX/bin/datacrumbs

    # Should show: cap_bpf,cap_perfmon+ep

Cleanup and Uninstallation
===========================

Remove sudoers configuration:

.. code-block:: bash

    sudo rm /etc/sudoers.d/datacrumbs

Remove Flux integration:

.. code-block:: bash

    # On all nodes
    sudo rm /etc/flux/system/prolog-job-manager.d/datacrumbs*
    sudo rm /etc/flux/system/epilog.d/datacrumbs*
    sudo rm /etc/systemd/system/datacrumbs@.service
    sudo systemctl daemon-reload

Remove Slurm integration:

.. code-block:: bash

    sudo rm /etc/slurm/prolog.d/datacrumbs.sh
    sudo rm /etc/slurm/epilog.d/datacrumbs.sh
    # Edit /etc/slurm/slurm.conf to remove Prolog/Epilog lines
    sudo systemctl restart slurmctld
