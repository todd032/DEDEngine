#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# shellcheck disable=SC1091
source "${ROOT_DIR}/tools/setup_web_toolchain.sh"

pushd "${ROOT_DIR}" >/dev/null
cmake --preset web-emscripten
cmake --build --preset web-release
popd >/dev/null

echo "Web build ready at ${ROOT_DIR}/build/web/site/index.html"
