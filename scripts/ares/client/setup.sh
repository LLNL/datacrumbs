#!/bin/bash
echo "$(date '+%Y-%m-%d %H:%M:%S') - Starting setup.sh"

export DATACRUMBS_DIR=/mnt/common/xcatcadmin/datacrumbs
export DATACRUMBS_INSTALL_DIR=/mnt/common/xcatcadmin/datacrumbs/install

echo "$(date '+%Y-%m-%d %H:%M:%S') - Setting PATH and LD_LIBRARY_PATH"
export PATH=$DATACRUMBS_INSTALL_DIR/bin:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/bin:$PATH
export LD_LIBRARY_PATH=$DATACRUMBS_INSTALL_DIR/lib:$DATACRUMBS_INSTALL_DIR/libexec/datacrumbs/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$DATACRUMBS_DIR:$PYTHONPATH

echo "$(date '+%Y-%m-%d %H:%M:%S') - setup.sh completed"
