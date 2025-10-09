#! /bin/bash

clang_format_exe="clang-format"
if [ $# -ge 1 ]; then
    clang_format_exe="$1"
fi

SUPPORTED_CLANG_FORMAT_VERSION="19.1.7"

if [ command -v $clang_format_exe >/dev/null 2>&1 ]; then
    echo "You must have 'clang-format' in PATH to use 'check-formatting.sh'"
    exit 1
fi

clang_format_version_str=$($clang_format_exe --version)
clang_format_version=$(echo "$clang_format_version_str" | grep -oP 'clang-format version \K\d+(\.\d+)+')

if [ "$clang_format_version" != "$SUPPORTED_CLANG_FORMAT_VERSION" ]; then
    echo "WARNING: the .clang-format file in this repo is designed for version $SUPPORTED_CLANG_FORMAT_VERSION."
    echo "         You are running with clang-format v$clang_format_version."
    echo "         The resulting check is highly likely to be incorrect."
fi

if [ command -v find >/dev/null 2>&1 ]; then
    echo "You must have 'find' in PATH to use 'check-formatting.sh'"
    exit 1
fi

if [ command -v dirname >/dev/null 2>&1 ]; then
    echo "You must have 'dirname' in PATH to use 'check-formatting.sh'"
    exit 1
fi

if [ command -v xargs >/dev/null 2>&1 ]; then
    echo "You must have 'dirname' in PATH to use 'check-formatting.sh'"
    exit 1
fi

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

curr_dir=$(pwd)

cd $SCRIPT_DIR
cd ..
cd ..

echo "Check formatting of C/C++ code in 'src'"
find src \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | xargs -0 -P "$(nproc)" $clang_format_exe --dry-run -Werror
src_check_rc=$?

echo "Check formatting of C/C++ code in 'etc/datacrumbs/plugins'"
find etc/datacrumbs/plugins \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | xargs -0 -P "$(nproc)" $clang_format_exe --dry-run -Werror
plugins_check_rc=$?

echo "Check formatting of C/C++ code in 'tests'"
find tests \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | xargs -0 -P "$(nproc)" $clang_format_exe --dry-run -Werror
tests_check_rc=$?

echo "Check formatting of C/C++ code in 'tools'"
find tools \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | xargs -0 -P "$(nproc)" $clang_format_exe --dry-run -Werror
tools_check_rc=$?

cd $curr_dir

if [ $src_check_rc -ne 0 ] || [ $plugins_check_rc -ne 0 ] || [ $tests_check_rc -ne 0 ] || [ $tools_check_rc -ne 0 ]; then
    echo "Some formatting checks failed. Please run 'autoformat.sh' to fix the issues."
    exit 2
else
    echo "All checks passed successfully."
    exit 0
fi