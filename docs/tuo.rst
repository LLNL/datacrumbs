
1. Modules to be loaded

    .. code-block:: bash
        
        module load gcc/11.2.0
        export CC=`which gcc`
        export CXX=`which g++`

2. BPF Time configuration for Tuo

    .. code-block:: bash

        export BPFTIME_SHM_MEMORY_MB=10240
        export BPFTIME_MAX_FD_COUNT=128000
        bpftime --install-location $PREFIX/lib load /usr/workspace/haridev/datacrumbs/build/bin/datacrumbs "run" "tuolumne-mpiio" "--user" "haridev" "--config_path" "/usr/workspace/haridev/datacrumbs/etc/datacrumbs/configs" "--data_dir" "/usr/workspace/haridev/datacrumbs/etc/datacrumbs/data" "--trace_log_dir" "/usr/workspace/haridev/datacrumbs/etc/datacrumbs/logs"
        add-auto-load-safe-path /opt/cray/pe/gcc/11.2.0/snos/lib64/libstdc++.so.6.0.29-gdb.py
        set follow-fork-mode child
        set detach-on-fork off
        set print-frame-arguments all