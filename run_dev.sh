#!/usr/bin/env bash
set -euo pipefail

# 允许脚本放在“项目根”或“scripts/”目录都能定位到根
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -d "$SCRIPT_DIR/backend" && -d "$SCRIPT_DIR/frontend" ]]; then
  ROOT_DIR="$SCRIPT_DIR"
elif [[ -d "$SCRIPT_DIR/../backend" && -d "$SCRIPT_DIR/../frontend" ]]; then
  ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
else
  echo "[FATAL] Cannot locate project root (backend/ & frontend/ not found)." >&2
  exit 1
fi

BACKEND_PORT="${1:-8080}"
FRONTEND_PORT="${2:-5173}"

cleanup() {
  if [[ -n "${backend_pid:-}" ]] && kill -0 "$backend_pid" 2>/dev/null; then
    echo "[CLEANUP] Stopping backend (PID $backend_pid)..."
    kill "$backend_pid" 2>/dev/null || true
    wait "$backend_pid" 2>/dev/null || true
  fi
  if [[ -n "${frontend_pid:-}" ]] && kill -0 "$frontend_pid" 2>/dev/null; then
    echo "[CLEANUP] Stopping frontend (PID $frontend_pid)..."
    kill "$frontend_pid" 2>/dev/null || true
    wait "$frontend_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT

export CLEAR_BOMB_BACKEND_PORT="$BACKEND_PORT"
export CLEAR_BOMB_FRONTEND_PORT="$FRONTEND_PORT"

echo "[INFO] Project root: $ROOT_DIR"
echo "[INFO] Backend port: $BACKEND_PORT"
echo "[INFO] Frontend port: $FRONTEND_PORT"

# 确保后端脚本可执行
chmod +x "$ROOT_DIR/backend/scripts/run_dev_server.sh"

echo "[START] Backend on :$BACKEND_PORT"
(
  set -x
  cd "$ROOT_DIR/backend"
  ./scripts/run_dev_server.sh "$BACKEND_PORT"
) &
backend_pid=$!

echo "[START] Frontend on :$FRONTEND_PORT"
(
  set -x
  cd "$ROOT_DIR/frontend"
  npm install
  npm run dev -- --host --port "$FRONTEND_PORT"
) &
frontend_pid=$!

# 等“全部”子进程结束，而不是等第一个
wait
