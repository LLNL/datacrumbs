#!/bin/bash
/opt/datacrumbs-build/bin/datacrumbs start docker --user root \
                    --config_path /opt/datacrumbs/etc/datacrumbs/configs \
                    --data_dir /opt/datacrumbs/etc/datacrumbs/data \
                    --trace_log_dir /opt/datacrumbs/etc/datacrumbs/logs
mkdir -p /opt/data/
rm -rf /opt/data/*
LD_PRELOAD=/opt/datacrumbs-build/lib64/libdatacrumbs_client.so dd if=/dev/zero of=/opt/data/img_temp.bat bs=1M count=16
/opt/datacrumbs-build/bin/datacrumbs stop docker --user root \
                    --config_path /opt/datacrumbs/etc/datacrumbs/configs \
                    --data_dir /opt/datacrumbs/etc/datacrumbs/data \
                    --trace_log_dir /opt/datacrumbs/etc/datacrumbs/logs
zcat /opt/datacrumbs/etc/datacrumbs/logs/*
exit 0