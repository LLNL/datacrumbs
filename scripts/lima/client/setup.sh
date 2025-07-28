#!/bin/bash
echo "$(date '+%Y-%m-%d %H:%M:%S') - Starting setup.sh"

export SPACK_DIR=/home/lima.linux/spack
export DATACRUMBS_DIR=/home/lima.linux/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/lima.linux/datacrumbs/install

echo "$(date '+%Y-%m-%d %H:%M:%S') - Sourcing Spack environment"
source /opt/spack/share/spack/setup-env.sh

echo "$(date '+%Y-%m-%d %H:%M:%S') - Setting PATH and LD_LIBRARY_PATH"
export HDF5_DIR=/home/lima.linux/apps/hdf5
export PATH=$DATACRUMBS_INSTALL_DIR/bin:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/bin:$HDF5_DIR/bin:$PATH
export LD_LIBRARY_PATH=$DATACRUMBS_INSTALL_DIR/lib:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/lib:$HDF5_DIR/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$DATACRUMBS_DIR:$PYTHONPATH

echo "$(date '+%Y-%m-%d %H:%M:%S') - Loading Spack packages"
#spack load hdf5@1.14.5

echo "$(date '+%Y-%m-%d %H:%M:%S') - Activating datacrumbs environment"
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
export LD_LIBRARY_PATH=/usr/lib/llvm-19/lib:$LD_LIBRARY_PATH
echo "$(date '+%Y-%m-%d %H:%M:%S') - setup.sh completed"