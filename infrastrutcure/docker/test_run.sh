#!/bin/bash
/opt/datacrumbs-build/bin/datacrumbs docker --user root \
                    --config_path /opt/datacrumbs/etc/datacrumbs/configs \
                    --data_dir /opt/datacrumbs/etc/datacrumbs/data \
                    --trace_log_dir /opt/datacrumbs/etc/datacrumbs/logs &
DATACRUMBS_PID=$!
sleep 60
mkdir -p /opt/data/
rm -rf /opt/data/*
LD_PRELOAD=/opt/datacrumbs-build/lib64/libdatacrumbs_client.so dd if=/dev/zero of=/opt/data/img_temp.bat bs=1M count=16
kill -SIGINT $DATACRUMBS_PID
wait $DATACRUMBS_PID
zcat /opt/datacrumbs/etc/datacrumbs/logs/*
exit 0