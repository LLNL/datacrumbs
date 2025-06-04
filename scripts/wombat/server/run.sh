#!/bin/bash

# Pass all script arguments to the next command
ARGS="$@"

${DATACRUMBS_DIR}/scripts/wombat/server/run_tool --module wombat $ARGS
