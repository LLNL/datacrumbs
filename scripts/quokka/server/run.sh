#!/bin/bash

# Pass all script arguments to the next command
ARGS="$@"

${DATACRUMBS_DIR}/scripts/${DATACRUMBS_SYSTEM}/server/run_tool ${DATACRUMBS_SYSTEM} $ARGS
