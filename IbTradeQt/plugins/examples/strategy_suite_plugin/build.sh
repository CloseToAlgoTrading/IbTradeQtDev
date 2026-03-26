#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
mkdir -p "${BUILD_DIR}"

g++ -std=c++17 -fPIC -shared \
  "${SCRIPT_DIR}/src/strategy_suite_plugin.cpp" \
  -I"${SCRIPT_DIR}/../../api" \
  -I"${SCRIPT_DIR}/../../sdk" \
  -o "${BUILD_DIR}/libibtrade_strategy_suite_plugin.so"

echo "Built ${BUILD_DIR}/libibtrade_strategy_suite_plugin.so"
