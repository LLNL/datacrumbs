#!/bin/bash
echo "$(date '+%Y-%m-%d %H:%M:%S') - Starting setup.sh"

export SPACK_DIR=/opt/spack
export DATACRUMBS_DIR=/home/cc/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/cc/datacrumbs/install

echo "$(date '+%Y-%m-%d %H:%M:%S') - Sourcing Spack environment"
source /opt/spack/share/spack/setup-env.sh

echo "$(date '+%Y-%m-%d %H:%M:%S') - Setting PATH and LD_LIBRARY_PATH"
export PATH=$DATACRUMBS_INSTALL_DIR/bin:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/bin:$PATH
export LD_LIBRARY_PATH=$DATACRUMBS_INSTALL_DIR/lib:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$DATACRUMBS_DIR:$PYTHONPATH

echo "$(date '+%Y-%m-%d %H:%M:%S') - Loading Spack packages"
spack load hdf5@1.14.5

echo "$(date '+%Y-%m-%d %H:%M:%S') - Activating datacrumbs environment"
source ${DATACRUMBS_INSTALL_DIR}/bin/activate

echo "$(date '+%Y-%m-%d %H:%M:%S') - Setting ulimit and BCC_PROBE_LIMIT"
export BCC_PROBE_LIMIT=1048576
ulimit -n ${BCC_PROBE_LIMIT}

echo "$(date '+%Y-%m-%d %H:%M:%S') - setup.sh completed"