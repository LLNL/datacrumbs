#!/bin/bash

DATACRUMBS_SO=${DATACRUMBS_INSTALL_DIR}/lib/libdatacrumbs.so
echo "$(date '+%Y-%m-%d %H:%M:%S') DATACRUMBS_SO=${DATACRUMBS_SO}"
LD_PRELOAD=${DATACRUMBS_SO} /home/lima.linux/datacrumbs/scripts/lima/client/tests/simple_write_test/create_hdf5_c

