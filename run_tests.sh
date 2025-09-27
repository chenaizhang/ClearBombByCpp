#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -d "$SCRIPT_DIR/backend" && -d "$SCRIPT_DIR/frontend" ]]; then
  ROOT_DIR="$SCRIPT_DIR"
elif [[ -d "$SCRIPT_DIR/../backend" && -d "$SCRIPT_DIR/../frontend" ]]; then
  ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
else
  echo "[FATAL] Cannot locate project root (backend/ & frontend/ not found)." >&2
  exit 1
fi

BUILD_TYPE="${CLEAR_BOMB_BUILD_TYPE:-Debug}"
SKIP_FRONTEND_LINT="${CLEAR_BOMB_SKIP_FRONTEND_LINT:-0}"
BACKEND_PORT="${1:-8080}"
FRONTEND_PORT="${2:-5173}"
BUILD_DIR="$ROOT_DIR/backend/build"

force_release_port() {
  local port="$1"
  if [[ -z "$port" ]]; then
    return
  fi
  if ! command -v lsof >/dev/null 2>&1; then
    return
  fi
  local pids
  pids="$(lsof -ti tcp:"$port" 2>/dev/null | tr '\n' ' ')"
  if [[ -n "$pids" ]]; then
    printf '[CLEANUP] Forcing TCP port %s free (PIDs: %s)\n' "$port" "$pids"
    # shellcheck disable=SC2086
    kill -9 $pids 2>/dev/null || true
  fi
}

cleanup() {
  force_release_port "$BACKEND_PORT"
  force_release_port "$FRONTEND_PORT"
}

trap cleanup EXIT INT TERM HUP

printf '[INFO] Project root: %s\n' "$ROOT_DIR"
printf '[INFO] Backend build dir: %s\n' "$BUILD_DIR"
printf '[INFO] CMake build type: %s\n' "$BUILD_TYPE"
printf '[INFO] Backend port: %s\n' "$BACKEND_PORT"
printf '[INFO] Frontend port: %s\n' "$FRONTEND_PORT"

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" || "${CLEAR_BOMB_FORCE_CONFIGURE:-0}" == "1" ]]; then
  printf '[INFO] Configuring backend with CMake...\n'
  (
    set -x
    cmake -S "$ROOT_DIR/backend" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  )
fi

printf '[INFO] Building backend tests...\n'
(
  set -x
  cmake --build "$BUILD_DIR" --target clear_bomb_tests
)

printf '[INFO] Running backend test suite...\n'
(
  set -x
  ctest --output-on-failure --test-dir "$BUILD_DIR"
)

if [[ "$SKIP_FRONTEND_LINT" != "1" ]]; then
  printf '[INFO] Running frontend lint...\n'
  (
    set -x
    cd "$ROOT_DIR/frontend"
    npm install
    npm run lint
  )
else
  printf '[INFO] Skipping frontend lint (CLEAR_BOMB_SKIP_FRONTEND_LINT=%s).\n' "$SKIP_FRONTEND_LINT"
fi

printf '[SUCCESS] Test run complete.\n'

printf '[INFO] Launching backend and frontend...\n'
exec "$ROOT_DIR/run_dev.sh" "$BACKEND_PORT" "$FRONTEND_PORT"
