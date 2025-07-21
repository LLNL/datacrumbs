# Lima eBPF Environment

This is for running an environment for development with Lima
 
## Usage
DataCrumbs  is currently unstable on aarch64. Install QEMU alongside Lima if you are using macOS.
To set up your Lima for the first time:
```bash
limactl start --network=lima:user-v2 --name=ebpf lima/ebpf.yaml
```

If you are coming back to it from later:

```bash
limactl start ebpf
```
To connect to the Lima shell:
```bash
limactl shell ebpf
```
## Setting up dependencies
### Environmental variables
```bash
export SPACK_DIR=/home/lima.linux/spack
export DATACRUMBS_DIR=/home/lima.linux/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/lima.linux/datacrumbs/install
```
### cloning the repo
```bash
git clone git@github.com:hariharan-devarajan/datacrumbs.git ${DATACRUMBS_DIR}
```
### Setup spack
```bash
cd ${DATACRUMBS_DIR}/scripts/lima
./install_env.sh
```
*If you set up Lima on aarch64, you might not be able to install HDF5 with spack. You can install with `apt` or from scratch.


### Python dependencies
```bash
python3 -m venv --system-site-packages ${DATACRUMBS_INSTALL_DIR}
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
cp ${DATACRUMBS_DIR}/requirements/requirement.chameleon.txt ${DATACRUMBS_DIR}/requirements/requirement.arta_lima.txt
pip install -r ${DATACRUMBS_DIR}/requirements/requirement.chameleon.txt
```
If the clang in Python is not compatible with the system's clang, upgrade the lower version to the exact higher version.

### setting up DataCrumbs
```bash
cd ${DATACRUMBS_DIR}
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} ..
make install -j
```
To make sure files are executable:
```bash
chmod +x ~/datacrumbs/install/libexec/datacrumbs/bin/*
```
## Running the tool
Check `${DATACRUMBS_DIR}/datacrumbs/configs/module/default.yaml` and make necessary changes or write your desired model.
*scripts in `lima/server` and `lima/clients` are using a HDF5 built from source.
```bash
sudo su
source ${DATACRUMBS_DIR}/scripts/lima/server/setup.sh
. ${DATACRUMBS_DIR}/scripts/lima/server/init.sh -g
```

### Starting Server

```bash
source ${DATACRUMBS_DIR}/scripts/lima/server/setup.sh
. ${DATACRUMBS_DIR}/scripts/lima/server/run.sh
```

### Running Simple Test

```bash
source ${DATACRUMBS_DIR}/scripts/lima/client/setup.sh
. ${DATACRUMBS_DIR}/scripts/lima/client/tests/test_hdf5.sh
```
## Clean Up

You can stop:

```bash
limactl stop ebpf
```

Or just nuke it!

```bash
limactl delete ebpf
```
