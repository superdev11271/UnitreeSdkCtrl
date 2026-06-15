#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${SCRIPT_DIR}/build/unitree_sdk_ctrl"

if [[ ! -x "$BIN" ]]; then
    echo "Binary not found: $BIN" >&2
    echo "Run ./build.sh first." >&2
    exit 1
fi

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <networkInterface> [--timeout-ms N]" >&2
    echo "Example: $0 eth0" >&2
    echo "Example: $0 eth0 --timeout-ms 1500" >&2
    exit 1
fi

exec "$BIN" "$@"
