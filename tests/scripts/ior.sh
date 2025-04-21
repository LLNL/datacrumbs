#!/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";
echo $SCRIPT_DIR
TEST_DIR=$(dirname $SCRIPT_DIR)
PROJECT_DIR=$(dirname $TEST_DIR)
PARENT_DIR=$(dirname $PROJECT_DIR)
DATA_DIR=${PROJECT_DIR}/build/data
DROP_CACHES=1
mkdir -p $DATA_DIR
rm -rf $DATA_DIR/*
BLOCK=$1
if [ -z "$1" ]; then
  BLOCK=8g
fi
TRANSFER_SIZE=$2
if [ -z "$2" ]; then
  TRANSFER_SIZE=1m
fi
TOOL=$3
if [ -z "$3" ]; then
  TOOL=DATACRUMBS
fi
if [ "$TOOL" == "DATACRUMBS" ]; then
  LD_PRELOAD_ARG="LD_PRELOAD=${PROJECT_DIR}/build/libdatacrumbs.so"
elif [ "$TOOL" == "DARSHAN" ]; then
  LD_PRELOAD_ARG="LD_PRELOAD=$DARSHAN_DIR/lib/libdarshan.so"
elif [ "$TOOL" == "DARSHAN-DXT" ]; then
  LD_PRELOAD_ARG="LD_PRELOAD=$DARSHAN_DIR/lib/libdarshan.so"
  export DARSHAN_ENABLE_DXT=1
else
  echo "Unknown tool"
  exit 1
fi
INTERFACE=$4
if [ -z "$4" ]; then
  INTERFACE=POSIX
fi
DIRECTIO=$5
if [ -z "$5" ]; then
  DIRECTIO=0
fi
if [ "$DIRECTIO" -eq "1" ]; then
  echo "Direct IO"
  export FLAGS="-o O_DIRECT=1"
else
  echo "Buffered IO"
  export FLAGS=""
fi
FPP=$6
if [ -z "$6" ]; then
  FPP=0
fi
if [ "$FPP" -eq "1" ]; then
  echo "FPP"
  export FLAGS="${FLAGS} -F"
else
  echo "Collective IO"
  export FLAGS="${FLAGS} -c"
fi

for i in {1..10}; do
  echo "Iteration $i"
  configuration="-o=${DATA_DIR}/test.bat-${TRANSFER_SIZE} -m -b=${BLOCK} -i 1 -d 10 -t=${TRANSFER_SIZE} -a ${INTERFACE} $FLAGS"
  echo "Running ${IOR_INSTALL_DIR}/bin/ior ${configuration} -w"
  start_time=$(date +%s)
  mpirun -n 24 $LD_PRELOAD_ARG ${IOR_DIR}/bin/ior ${configuration} -w
  end_time=$(date +%s)
  elapsed_time=$((end_time - start_time))
  echo "Time taken for write iteration:$i: ${elapsed_time} seconds"
  sleep 10
  if [ "$DROP_CACHES" -eq "1" ];
  then
    echo "Clean Cache"
    sudo sh -c "/usr/bin/echo 3 > /proc/sys/vm/drop_caches"
  fi
  echo "Running ${IOR_INSTALL_DIR}/bin/ior ${configuration} -r"
  start_time=$(date +%s)
  mpirun -n 24 $LD_PRELOAD_ARG ${IOR_DIR}/bin/ior ${configuration} -r
  elapsed_time=$((end_time - start_time))
  echo "Time taken for read iteration:$i: ${elapsed_time} seconds"
done