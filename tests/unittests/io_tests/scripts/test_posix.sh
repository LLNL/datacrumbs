#!/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";
echo $SCRIPT_DIR
TEST_DIR=$(dirname $SCRIPT_DIR)
PROJECT_DIR=$(dirname $TEST_DIR)

DATACRUMBS_SO=${PROJECT_DIR}/build/libdatacrumbs.so

DATA_DIR=${PROJECT_DIR}/build/data
mkdir -p $DATA_DIR
NUM_FILES=1
NUM_OPS=$1
if [ -z "$1" ]; then
  NUM_OPS=1
fi
PROC=1
DIRECTIO=0
TEST_CASE=0 #write=0 read=1 both=2
CLEAN_PAGE_CACHE=$2
if [ -z "$2" ]; then
  CLEAN_PAGE_CACHE=0
fi
if [ "$TEST_CASE" -eq "0" ] || [ "$TEST_CASE" -eq "2" ]; then
  echo "Cleaning Data"
  ls -lhs $DATA_DIR
  rm -rf $DATA_DIR/*
fi

for TSKB in $((1*1024)); #1 4 16 64 256 1024 4096 16384 65536 262144
do
  TS=$((TSKB * 1024))
  if [ "$TEST_CASE" -eq "0" ] || [ "$TEST_CASE" -eq "2" ]; then
    cmd=(${PROJECT_DIR}/build/tests/df_tracer_test ${NUM_FILES} ${NUM_OPS} ${TS} ${DATA_DIR} 0 ${DIRECTIO})
    echo "${cmd[@]}"
    LD_PRELOAD=${DATACRUMBS_SO} "${cmd[@]}"
  fi
  if [ "$CLEAN_PAGE_CACHE" -eq "1" ]; then
    echo "Cleaning Cache"
    sudo sh -c "/usr/bin/echo 3 > /proc/sys/vm/drop_caches"
  fi
  if [ "$TEST_CASE" -eq "1" ] || [ "$TEST_CASE" -eq "2" ]; then
    sleep 5
    cmd=( ${PROJECT_DIR}/build/tests/df_tracer_test ${NUM_FILES} ${NUM_OPS} ${TS} ${DATA_DIR} 1 ${DIRECTIO})
    echo "${cmd[@]}"
    LD_PRELOAD=${DATACRUMBS_SO} "${cmd[@]}"
  fi
done


