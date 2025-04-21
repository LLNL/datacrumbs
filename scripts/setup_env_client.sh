#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJECT_DIR=$(dirname  $SCRIPT_DIR)
export PYTHONPATH=$PROJECT_DIR
SPACK_ROOT=/opt/spack
source ${SPACK_ROOT}/share/spack/setup-env.sh
#spack install openmpi@5.0.5%gcc@11.4.0 hdf5@1.14.5%gcc@11.4.0
#HDF5_DIR=$(spack location -i hdf5)
export HDF5_DIR=$(spack location -i hdf5@1.14.5%gcc@11.4.0)
export OPENMPI_DIR=$(spack location -i openmpi@5.0.5%gcc@11.4.0)
export DARSHAN_DIR=$(spack location -i darshan@3.4.6%gcc@11.4.0)
export IOR_DIR=$(spack location -i ior@4.0.0%gcc@11.4.0)
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HDF5_DIR/lib
echo "Set HDF5 to ${HDF5_DIR}"
echo "Set OPENMPI to ${OPENMPI_DIR}"
echo "Set DARSHAN_DIR to ${DARSHAN_DIR}"
echo "Set IOR_DIR to ${IOR_DIR}"