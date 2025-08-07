#!/bin/bash

DATACRUMBS_SO=${DATACRUMBS_INSTALL_DIR}/lib/libdatacrumbs_client.so
echo "$(date '+%Y-%m-%d %H:%M:%S') DATACRUMBS_SO=${DATACRUMBS_SO}"
PYTHON_DIR=/usr/bin/python3.9 #$(spack location -i python@3.10.16)
echo "$(date '+%Y-%m-%d %H:%M:%S') Run Py test ${PYTHON_DIR}"
cmd=(dlio_benchmark workload=unet3d_a100.yaml ++workload.dataset.data_folder=/tmp/dlio/data/unet3d ++workload.workflow.train=True ++workload.workflow.generate_data=False  ++workload.checkpoint.checkpoint_folder=/tmp/dlio/checkpoint/unet3d ++workload.train.epochs=1)
echo "${cmd[@]}"
export LD_PRELOAD=${DATACRUMBS_SO}
"${cmd[@]}"
unset LD_PRELOAD
echo "$(date '+%Y-%m-%d %H:%M:%S') Finished Py test"