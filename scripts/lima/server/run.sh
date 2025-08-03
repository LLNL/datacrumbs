#!/bin/bash

# Pass all script arguments to the next command
ARGS="$@"

${DATACRUMBS_DIR}/scripts/lima/server/run_tool lima $ARGS --log_file "${DATACRUMBS_DIR}/logs/lima-run.log"
