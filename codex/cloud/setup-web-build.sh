#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASHRC_FILE="${HOME}/.bashrc"
MARKER="# dedengine-web-build"

# shellcheck disable=SC1091
source "${ROOT_DIR}/tools/setup_web_toolchain.sh"

if ! grep -Fq "${MARKER}" "${BASHRC_FILE}" 2>/dev/null; then
  cat >> "${BASHRC_FILE}" <<EOF
${MARKER}
export PATH="\$HOME/.local/bin:\$PATH"
if [ -f "${ROOT_DIR}/.external-cache/emsdk/emsdk_env.sh" ]; then
  source "${ROOT_DIR}/.external-cache/emsdk/emsdk_env.sh" >/dev/null
fi
EOF
fi

echo "Codex Cloud web build environment is ready."
