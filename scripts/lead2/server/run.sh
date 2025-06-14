#!/bin/bash

# Pass all script arguments to the next command
ARGS="$@"

${DATACRUMBS_DIR}/scripts/lead2/server/run_tool --module lead2 --log_file "${DATACRUMBS_DIR}/logs/lead2-run.log" $ARGS
