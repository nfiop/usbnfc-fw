#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")"

VENV=".venv" 
SCRIPT="user/nandctl/main.py"

if [[ ! -d "$VENV" ]]; then 
    echo "Creating virtual environment..." 
    python3 -m venv "$VENV" 
fi

PYTHON="$VENV/bin/python"

echo "Installing dependencies..." 

"$PYTHON" -m pip install -q pyserial cbor2 cobs

echo "Starting RP2040 CLI..." 

exec $PYTHON $SCRIPT "$@"
