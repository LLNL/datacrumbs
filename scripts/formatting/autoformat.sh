#! /bin/bash

clang_format_exe="clang-format"
if [[ $# -ge 1 ]]; then
  clang_format_exe="$1"
fi

SUPPORTED_CLANG_FORMAT_VERSION="19.1.7"

if ! command -v $clang_format_exe >/dev/null 2>&1; then
  echo "You must have 'clang-format' in PATH to use 'autoformat.sh'"
  exit 1
fi

clang_format_version_str=$($clang_format_exe --version)
clang_format_version=$(echo "$clang_format_version_str" | grep -oP 'clang-format version \K\d+(\.\d+)+')

if [ "$clang_format_version" != "$SUPPORTED_CLANG_FORMAT_VERSION" ]; then
  echo "WARNING: the .clang-format file in this repo is designed for version 18.1.8."
  echo "         You are running with clang-format v$clang_format_version."
  echo "         The resulting formatting is highly likely to be incorrect."
fi

if ! command -v find >/dev/null 2>&1; then
  echo "You must have 'find' in PATH to use 'autoformat.sh'"
  exit 1
fi

if ! command -v dirname >/dev/null 2>&1; then
  echo "You must have 'dirname' in PATH to use 'autoformat.sh'"
  exit 1
fi

if ! command -v xargs >/dev/null 2>&1; then
  echo "You must have 'xargs' in PATH to use 'autoformat.sh'"
  exit 1
fi

if ! command -v shfmt >/dev/null 2>&1; then
  echo "You must have 'shfmt' in PATH to use 'autoformat.sh'"
  exit 1
fi

if ! command -v cmake-format >/dev/null 2>&1; then
  echo "You must have 'cmake-format' in PATH to use 'autoformat.sh'"
  exit 1
fi

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

curr_dir=$(pwd)

cd $SCRIPT_DIR
cd ..
cd ..


echo "Check formatting of C/C++ code in '.'"
find . \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \( -path "./src/*" -o -path "./tools/*" -o -path "./tests/*" -o -path "./etc/datacrumbs/plugins/*" \) -print0 | xargs -0 -P "$(nproc)" $clang_format_exe -i

echo "Formatting shell scripts in '.'"
find . \( -name "*.sh" -o -name "*.in" \) \( -path "./etc/datacrumbs/data/*" -o -path "./infrastructure/docker/*" -o -path "./scripts/datacrumbs/*" -o -path "./tests/*" -o -path "./tools/*" \) -not -path "*.txt.in" -print0 | xargs -0 shfmt -w

echo "Formatting CMake files in '.'"
find . \( -name "CMakeLists.txt*" -o -name "*.cmake" \) \( -path "./src/*" -o -path "./tools/*" -o -path "./tests/*" -o -path "./cmake/*" \) -print0 | xargs -0 cmake-format -i

cd $curr_dir
