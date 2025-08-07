#!/bin/bash
echo "$(date '+%Y-%m-%d %H:%M:%S') - Starting setup.sh"

export SPACK_DIR=/home/haridev/spack
export DATACRUMBS_DIR=/home/haridev/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/haridev/datacrumbs/build

echo "$(date '+%Y-%m-%d %H:%M:%S') - Sourcing Spack environment"
source ${SPACK_DIR}/share/spack/setup-env.sh

echo "$(date '+%Y-%m-%d %H:%M:%S') - Setting PATH and LD_LIBRARY_PATH"
export PATH=$DATACRUMBS_INSTALL_DIR/bin:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/bin:$PATH
export LD_LIBRARY_PATH=$DATACRUMBS_INSTALL_DIR/lib:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/lib:$LD_LIBRARY_PATH

# echo "$(date '+%Y-%m-%d %H:%M:%S') - Loading Spack packages"
# spack load llvm@19.1.7 hdf5@1.14.5 ior@4.0.0 openmpi@5.0.7 cmake@3.26.5
# spack load gcc@11.5.0

echo "$(date '+%Y-%m-%d %H:%M:%S') - setup.sh completed"