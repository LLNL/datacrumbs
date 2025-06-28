#!/bin/bash

DATACRUMBS_SO=${DATACRUMBS_INSTALL_DIR}/lib/libdatacrumbs.so
export DATACRUMBS_USDT_ENABLE=1
echo "$(date '+%Y-%m-%d %H:%M:%S') DATACRUMBS_SO=${DATACRUMBS_SO}"
PYTHON_DIR=/usr/bin/python #$(spack location -i python@3.10.16)
echo "$(date '+%Y-%m-%d %H:%M:%S') Run Py test ${PYTHON_DIR}"
cmd=(${PYTHON_DIR} ${DATACRUMBS_DIR}/tests/test.py)
echo "${cmd[@]}"
LD_PRELOAD=${DATACRUMBS_SO} "${cmd[@]}"
echo "$(date '+%Y-%m-%d %H:%M:%S') Finished Py test"