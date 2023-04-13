#!/bin/bash
# Create an empty array to store found files
found_files=()
# Loop through the input files and check if they exist in the directory tree rooted at ~
for file in "$@"; do
  file_path=$(find ~ -type f -name "${file}" -print -quit)
  if [ -n "${file_path}" ]; then
    found_files+=("${file_path}")
  fi
done
# If found files array is not empty, create temp.tar.gz containing the found files
if [ ${#found_files[@]} -gt 0 ]; then
  tar czf temp.tar.gz "${found_files[@]}"
  echo "temp.tar.gz created with the found files."
else
  echo "No file found"
fi

