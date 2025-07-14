#!/bin/bash

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

log "Checking environment variables..."
echo "SPACK_DIR=${SPACK_DIR}"
echo "DATACRUMBS_DIR=${DATACRUMBS_DIR}"
echo "DATACRUMBS_INSTALL_DIR=${DATACRUMBS_INSTALL_DIR}"
read -p "Are these environment variables correct? (y/n): " confirm
if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "Aborting installation."
    exit 1
fi

log "Updating apt-get..."
sudo apt-get update

log "Installing system packages: hwloc, libtool, openssl, libssl-dev, gfortran, gcc, g++..."
sudo apt-get install -y hwloc libtool openssl libssl-dev gfortran gcc g++

log "Changing permissions for /opt..."
sudo chmod 777 /opt -R

log "Cloning Spack repository into ${SPACK_DIR}..."
git clone --depth=2  https://github.com/spack/spack.git ${SPACK_DIR}

log "Sourcing Spack environment..."
source ${SPACK_DIR}/share/spack/setup-env.sh 

log "Finding external packages with Spack..."
spack external find

log "Finding available compilers with Spack..."
spack compiler find

log "Installing HDF5 1.14.5 with GCC 11.4.0 using Spack..."
spack install -j64 hdf5@1.14.5%gcc@11.4.0

log "Creating symlink for HDF5 installation in ${DATACRUMBS_INSTALL_DIR}..."
spack view --verbose symlink ${DATACRUMBS_INSTALL_DIR} hdf5@1.14.5%gcc@11.4.0

export HDF5_DIR=$(spack location -i hdf5@1.14.5%gcc@11.4.0)

log "Installed HDF5 into ${HDF5_DIR} and created a symlink on ${DATACRUMBS_INSTALL_DIR}"