# Readme for Lead2

Location of spack is `/home/haridev/spack`

## Manage Environments

```bash
export SPACK_DIR=/home/haridev/spack
export DATACRUMBS_DIR=/home/haridev/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/haridev/datacrumbs/install
```

## Configure Spack

```bash
source ${SPACK_DIR}/share/spack/setup-env.sh
```
Spack does the following

- GCC 11.5 is picked from system
- openmpi is installed into system
- Python 3.9 is picked from system

## Installation

### Load Dependency

```bash
source ${SPACK_DIR}/share/spack/setup-env.sh
spack load llvm@19.1.7 hdf5@1.14.5 ior@4.0.0 openmpi@5.0.7 cmake@3.26.5
spack load gcc@11.5.0 
```

### DataCrumbs

All datacrumbs executables and dependencies would be installed in DATACRUMBS_INSTALL_DIR



```bash
cd /home/haridev
git clone git@github.com:hariharan-devarajan/datacrumbs.git
cd datacrumbs
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} ..
make install -j
cd -
```

### Test Dependencies

#### Activate project spack

```bash
source ${SPACK_DIR}$/share/spack/setup-env.sh
```

```bash
spack view --verbose symlink ${DATACRUMBS_INSTALL_DIR} ior@4.0.0+hdf5 ^hdf5@1.14.5
```

## Running the tool

### Generating Probes

```bash
export SPACK_DIR=/home/haridev/spack
export DATACRUMBS_DIR=/home/haridev/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/haridev/datacrumbs/install
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
source ${DATACRUMBS_DIR}/scripts/lead2/server/setup.sh
${DATACRUMBS_DIR}/scripts/lead2/server/init.sh -g
```

### Starting Server

```bash
sudo su

export SPACK_DIR=/home/haridev/spack
export DATACRUMBS_DIR=/home/haridev/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/haridev/datacrumbs/install
source ${DATACRUMBS_DIR}/scripts/lead2/server/setup.sh
${DATACRUMBS_DIR}/scripts/lead2/server/init.sh
${DATACRUMBS_DIR}/scripts/lead2/server/run.sh
```

### Running Simple Test

```bash
export SPACK_DIR=/home/haridev/spack
export DATACRUMBS_DIR=/home/haridev/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/haridev/datacrumbs/install
source ${DATACRUMBS_DIR}/scripts/lead2/client/setup.sh
${DATACRUMBS_DIR}/scripts/lead2/client/tests/test_posix.sh
```