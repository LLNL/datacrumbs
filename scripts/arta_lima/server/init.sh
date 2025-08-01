#!/bin/bash
GEN_PROBES=true

while getopts "gh" opt; do
    case $opt in
        g)
            GEN_PROBES=true
            ;;
        h)
            echo "Usage: $0 [-g] [-h]"
            echo "  -g    Generate probes"
            echo "  -h    Show this help message"
            exit 0
            ;;
        *)
            echo "Invalid option: -$OPTARG" >&2
            echo "Use -h for help."
            exit 1
            ;;
    esac
done

if [ "$GEN_PROBES" = true ]; then
    ${DATACRUMBS_DIR}/scripts/arta_lima/server/initialize_tool arta_lima --generate_probes
else
    ${DATACRUMBS_DIR}/scripts/arta_lima/server/initialize_tool arta_lima 
fi
