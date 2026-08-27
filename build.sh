#!/bin/sh
set -e
cd "$(dirname "$0")"
cmake -B build
cmake --build build -j$(nproc)
echo "构建完成: build/"
