#!/bin/bash

# A script to split a .pfw file into multiple files based on unique PIDs.
# --- Usage ---
# ./refractor.sh <your_file.pfw>

# 1. Validate input
if [ -z "$1" ]; then
    echo "Error: No input file specified."
    echo "Usage: $0 <input_file.pfw>"
    exit 1
fi

input_file="$1"

if [ ! -f "$input_file" ]; then
    echo "Error: File '$input_file' not found."
    exit 1
fi

echo "Processing file: $input_file"

# 2. Extract unique PIDs from the file.
unique_pids=$(grep -oP '"pid":\K[0-9]+' "$input_file" | sort -u)

# Check if any PIDs were found
if [ -z "$unique_pids" ]; then
    echo "No PIDs found in the file. Exiting."
    exit 0
fi

echo "Found unique PIDs: $unique_pids"

# 3. Loop through each unique PID and create a corresponding output file.
for pid in $unique_pids; do
    # Define the output filename based on the pid
    output_file="output_${pid}.pfw"
    echo "-> Creating file for PID $pid: $output_file"

    # 4. Use grep to find all lines containing the exact PID and save them to the new file.
    grep "\"pid\":${pid}[,]" "$input_file" > "$output_file"
done

echo "Splitting complete."
