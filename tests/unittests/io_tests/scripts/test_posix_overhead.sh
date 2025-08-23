#!/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";
echo $SCRIPT_DIR
TEST_DIR=$(dirname $SCRIPT_DIR)
PROJECT_DIR=$(dirname $TEST_DIR)

DATACRUMBS_SO=${PROJECT_DIR}/build/libdatacrumbs.so

DATA_DIR=${PROJECT_DIR}/build/data
mkdir -p $DATA_DIR
NUM_FILES=1
NUM_OPS=$((100))
PROC=2
DIRECTIO=0
TEST_CASE=0 #write=0 read=1 both=2
if [ "$TEST_CASE" -eq "0" ] || [ "$TEST_CASE" -eq "2" ]; then
  echo "Cleaning Data"
  ls -lhs $DATA_DIR
  rm -rf $DATA_DIR/*
fi

echo "Base"
TS=$((1*1024 * 1024))
for i in {1..5};
do
cmd="mpirun -np ${PROC} --use-hwthread-cpus ${PROJECT_DIR}/build/tests/df_tracer_test ${NUM_FILES} ${NUM_OPS} ${TS} ${DATA_DIR} ${TEST_CASE} ${DIRECTIO}"
# echo $cmd
$cmd
done
sleep 5
echo "DataCrums"
for i in {1..5};
do
cmd="mpirun -np ${PROC} --use-hwthread-cpus -x LD_PRELOAD=${DATACRUMBS_SO} ${PROJECT_DIR}/build/tests/df_tracer_test ${NUM_FILES} ${NUM_OPS} ${TS} ${DATA_DIR} ${TEST_CASE} ${DIRECTIO}"
# echo $cmd
$cmd
done


