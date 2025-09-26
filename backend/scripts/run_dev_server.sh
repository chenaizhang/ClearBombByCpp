#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/build"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" --target clear_bomb_server
"${BUILD_DIR}/clear_bomb_server" "$@"
