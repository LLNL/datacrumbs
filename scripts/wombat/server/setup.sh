#!/bin/bash
echo "$(date '+%Y-%m-%d %H:%M:%S') - Starting setup.sh"

export SPACK_DIR=/proj/csc671/proj_shared/spack
export DATACRUMBS_DIR=/proj/csc671/proj_shared/datacrumbs
export DATACRUMBS_INSTALL_DIR=/proj/csc671/proj_shared/datacrumbs/install

echo "$(date '+%Y-%m-%d %H:%M:%S') - Sourcing Spack environment"
source /proj/csc671/proj_shared/spack/share/spack/setup-env.sh

echo "$(date '+%Y-%m-%d %H:%M:%S') - Setting PATH and LD_LIBRARY_PATH"
export PATH=$DATACRUMBS_INSTALL_DIR/bin:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/bin:$PATH
export LD_LIBRARY_PATH=$DATACRUMBS_INSTALL_DIR/lib:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$DATACRUMBS_DIR:$PYTHONPATH

echo "$(date '+%Y-%m-%d %H:%M:%S') - Loading Spack packages"
spack load llvm@19.1.7 hdf5@1.14.5  ior@4.0.0 openmpi@5.0.5 cmake@3.30.2
spack load gcc@13.2.0

echo "$(date '+%Y-%m-%d %H:%M:%S') - Activating datacrumbs environment"
source ${DATACRUMBS_INSTALL_DIR}/bin/activate

echo "$(date '+%Y-%m-%d %H:%M:%S') - Setting ulimit and BCC_PROBE_LIMIT"
ulimit -n 1048576
export BCC_PROBE_LIMIT=1048576

echo "$(date '+%Y-%m-%d %H:%M:%S') - setup.sh completed"