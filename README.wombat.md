# Readme for Wombat

Location of Wombat system spack is `/autofs/nccs-svm1_wombat_sw/RHEL9.4/spack`

Location of project_shared spack is `/proj/csc671/proj_shared/spack`

GCC, Python, and OpenMPI have been added from system spack to project_spack to make sure we use the correct compilers for our dependency.

## Installation

### DataCrumbs

All datacrumbs executables and dependencies would be installed in DATACRUMBS_INSTALL_DIR

```bash
export DATACRUMBS_DIR=/proj/csc671/proj_shared/datacrumbs
export DATACRUMBS_INSTALL_DIR=/proj/csc671/proj_shared/datacrumbs/install
```

```bash
cd /proj/csc671/proj_shared/
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
source /proj/csc671/proj_shared/spack/share/spack/setup-env.sh
```

```bash
spack view --verbose symlink ${DATACRUMBS_INSTALL_DIR} ior@4.0.0+hdf5 ^hdf5@1.14.5
```

```bash
spack load llvm@19.1.7 hdf5@1.14.5  ior@4.0.0 openmpi@5.0.5 cmake@3.30.2
spack load gcc@13.2.0 
```

### Install Python Environment

```bash
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
pip install --upgrade pip
pip install -r ${DATACRUMBS_DIR}/requirements.txt
pip install -r ${DATACRUMBS_DIR}/bcc_requirements.txt
```

### Install BCC

```bash
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
BCC_HOME=/proj/csc671/proj_shared/bcc
git clone https://github.com/iovisor/bcc.git ${BCC_HOME}
pushd ${BCC_HOME}
mkdir build
pushd build
cmake -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} ..
make -j
make install -j
cmake -DPYTHON_CMD=python -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} .. # build python3 binding
pushd src/python/
make
make install
popd
popd
popd
cp -r ${BCC_HOME}/build/src/python/bcc-python3/bcc/* ${DATACRUMBS_INSTALL_DIR}/lib/python3.12/site-packages/bcc/
```

## Running the tool

### Starting Server

```bash
export DATACRUMBS_DIR=/proj/csc671/proj_shared/datacrumbs
source ${DATACRUMBS_DIR}/scripts/wombat/server/setup.sh
source ${DATACRUMBS_DIR}/scripts/wombat/server/init.sh
source ${DATACRUMBS_DIR}/scripts/wombat/server/run.sh
```

### Running Simple Test

```bash
export DATACRUMBS_DIR=/proj/csc671/proj_shared/datacrumbs
export DATACRUMBS_INSTALL_DIR=/proj/csc671/proj_shared/datacrumbs/install
source ${DATACRUMBS_DIR}/scripts/wombat/client/setup.sh
source ${DATACRUMBS_DIR}/scripts/wombat/client/tests/test_posix.sh
```