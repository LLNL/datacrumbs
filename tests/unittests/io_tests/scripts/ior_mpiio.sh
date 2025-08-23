#!/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";
echo $SCRIPT_DIR
TEST_DIR=$(dirname $SCRIPT_DIR)
PROJECT_DIR=$(dirname $TEST_DIR)
PARENT_DIR=$(dirname $PROJECT_DIR)
#IOR_INSTALL_DIR=/opt/spack/opt/spack/linux-ubuntu22.04-icelake/gcc-11.4.0/ior-4.0.0-arszr4x4i7xuua4opbyx73oqq7tlzljo
IOR_INSTALL_DIR=$(spack location -i ior@4.0.0%gcc@11.4.0)
DATACRUMBS_SO=${PROJECT_DIR}/build/libdatacrumbs.so

DATA_DIR=${PROJECT_DIR}/build/data
DROP_CACHES=1
mkdir -p $DATA_DIR
rm -rf $DATA_DIR/*
BLOCK=32m
PROC=10
ts=1m
cmd=(mpirun -np ${PROC} -x LD_PRELOAD=$DATACRUMBS_SO  ${IOR_INSTALL_DIR}/bin/ior -o=${DATA_DIR}/test-${ts} -b=${BLOCK} -i=1 -t=${ts} -a=MPIIO -c)
echo "${cmd[@]}"
"${cmd[@]}"