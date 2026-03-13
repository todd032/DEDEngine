#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CACHE_DIR="${ROOT_DIR}/.external-cache"
EMSDK_DIR="${CACHE_DIR}/emsdk"
EMSDK_VERSION="${EMSDK_VERSION:-latest}"

mkdir -p "${CACHE_DIR}"

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is required to install the web toolchain." >&2
  exit 1
fi

export PATH="${HOME}/.local/bin:${PATH}"

if ! command -v cmake >/dev/null 2>&1; then
  python3 -m pip install --user cmake
fi

if ! command -v ninja >/dev/null 2>&1; then
  python3 -m pip install --user ninja
fi

if [ ! -d "${EMSDK_DIR}" ]; then
  git clone https://github.com/emscripten-core/emsdk.git "${EMSDK_DIR}"
fi

pushd "${EMSDK_DIR}" >/dev/null
./emsdk install "${EMSDK_VERSION}"
./emsdk activate "${EMSDK_VERSION}"
# shellcheck disable=SC1091
source "${EMSDK_DIR}/emsdk_env.sh" >/dev/null
popd >/dev/null

echo "Web toolchain ready:"
echo "  cmake: $(command -v cmake)"
echo "  ninja: $(command -v ninja)"
echo "  emcc: $(command -v emcc)"
