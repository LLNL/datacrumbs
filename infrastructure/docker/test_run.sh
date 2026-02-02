#!/bin/bash
set -x
source /opt/datacrumbs-install/bin/datacrumbs_setup --verbose
datacrumbs_create_log_dir --verbose
datacrumbs_run --verbose --app "datacrumbs_wrap dd if=/dev/zero of=/tmp/img_temp.bat bs=1M count=16"
