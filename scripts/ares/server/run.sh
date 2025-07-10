#!/bin/bash

# Pass all script arguments to the next command
ARGS="$@"

${DATACRUMBS_DIR}/scripts/ares/server/run_tool ares --log_file "${DATACRUMBS_DIR}/logs/ares-run.log" $ARGS

