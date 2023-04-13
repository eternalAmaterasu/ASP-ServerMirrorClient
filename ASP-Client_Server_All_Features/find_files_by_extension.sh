#!/bin/bash

# Collect files with the specified extensions
matching_files=()
for ext in "$@"
do
  while IFS= read -r -d '' file; do
    matching_files+=("$file")
  done < <(find ~ -type f -name "*.${ext}" -print0)
done

# Create the archive only if there are matching files
if [ ${#matching_files[@]} -gt 0 ]; then
  # Create an empty tar archive
  tar -czf temp.tar.gz --files-from /dev/null

  # Add the matching files to the archive
  printf '%s\0' "${matching_files[@]}" | tar -czvf temp.tar.gz --null -T -
fi

