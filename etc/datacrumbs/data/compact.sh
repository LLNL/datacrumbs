#!/bin/bash

for file in *.json; do
    [ -e "$file" ] || continue
    jq -c . "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done