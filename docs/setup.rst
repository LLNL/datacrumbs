=================
Runtime Setup
=================

This page covers the runtime layout that `datacrumbs` expects after install.

Environment and Module Setup
============================

The user-facing setup helpers now live in `datacrumbs-utils`, but they are
installed into the shared DataCrumbs prefix.

Typical setup:

.. code-block:: bash

    module use <install-prefix>/etc/datacrumbs/lmod/modulefiles
    module load datacrumbs/<version>

or:

.. code-block:: bash

    source <install-prefix>/bin/datacrumbs_setup

Important Runtime Inputs
========================

At runtime, `datacrumbs` needs:

- a signed probes file passed on the command line
- the installed system configuration file
- the installed probe secret

Those installed files are:

- `<install-prefix>/share/datacrumbs/data/system-probe-<install-user>-<host>.json.gz`
- `<install-prefix>/share/datacrumbs/data/.datacrumbs-probe-secret`

On login nodes, probe generation also depends on:

- `<install-prefix>/etc/datacrumbs/systemd/datacrumbs_sign_probes.service`

Direct Runtime Invocation
=========================

The current runtime interface is:

.. code-block:: bash

    datacrumbs <signed-probes.json.gz> [run-id]

If `run-id` is omitted, DataCrumbs resolves it in this order:

1. explicit CLI argument
2. environment variable named by `DATACRUMBS_JOB_ID_VAR`
3. generated timestamp

Trace and Run Files
===================

Runtime configuration derives and manages:

- trace directory from `DATACRUMBS_TRACE_DIR_PATTERN`
- run-id file from `DATACRUMBS_SERVER_RUN_ID_FILE`
- ready file from `DATACRUMBS_SERVER_RUN_DIR`

The ready file is:

.. code-block:: text

    <run-dir>/datacrumbs-<run-id>.ready

It is written once the daemon reaches the ready point and removed on shutdown.

Resource Limits
===============

During startup, `datacrumbs` raises these soft limits to their hard limits on a
best-effort basis:

- `RLIMIT_NOFILE`
- `RLIMIT_AS`
- `RLIMIT_MEMLOCK`

Service Logs
============

When launched through `datacrumbs@<run-id>.service`, stdout and stderr are
redirected to:

.. code-block:: text

    ${DATACRUMBS_LOG_DIR}/datacrumbs_${DATACRUMBS_USER}_${DATACRUMBS_SERVICE_RUN_ID}_$(hostname).log

Runtime-Detected Invalid Probes
===============================

Failed runtime attach targets are recorded to a compressed invalid-probe
database:

.. code-block:: text

    <install-prefix>/share/datacrumbs/data/probes-invalid-<install-user>-<config-name>.json.gz

This file is written with mode `0400` and owned by `root`.

Probe signing service
=====================

`datacrumbs_probe_configurator` no longer signs probe files directly with a
shared user-readable secret. Instead, it connects to the trusted login-node
signer service:

.. code-block:: bash

    sudo ln -sf <install-prefix>/etc/datacrumbs/systemd/datacrumbs_sign_probes.service \
      /etc/systemd/system/datacrumbs_sign_probes.service
    sudo systemctl daemon-reload
    sudo systemctl enable --now datacrumbs_sign_probes.service

That service runs as `root`.
It only accepts local requests from the installed
`datacrumbs_probe_configurator_exec` binary.
Subsequent runs use it to skip known invalid targets before attach.
