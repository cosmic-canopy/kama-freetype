#!/usr/bin/env bash
# test-wasm.sh — the same tests built for wasm (emscripten) and run under node. The fonts are embedded into the
# module's virtual file system at their package-relative paths (tests/kama.json, `select.TARGET.WASM`).
#
# This is the leg that proves the vendoring: a wasm build has no system FreeType, so a package that linked
# `-lfreetype` could not run here at all. The sources are identical to the native run, with the same wrapper units,
# the same options and the same modules.
#
#   KAMA   the kama compiler                   (default: kama on the PATH)
#   EMSDK  an emsdk to activate                (default: ~/emsdk; skipped when emcc is already on the PATH)
#   NODE   the node that runs the module       (default: node on the PATH once emsdk is active)
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
KAMA=${KAMA:-kama}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cd "$ROOT"

if ! command -v emcc >/dev/null 2>&1; then
    EMSDK=${EMSDK:-$HOME/emsdk}
    [ -f "$EMSDK/emsdk_env.sh" ] || { echo "test-wasm.sh: no emcc on the PATH and no $EMSDK/emsdk_env.sh" >&2; exit 1; }
    . "$EMSDK/emsdk_env.sh" >/dev/null 2>&1
fi
NODE=${NODE:-node}
command -v "$NODE" >/dev/null 2>&1 || { echo "test-wasm.sh: no node" >&2; exit 1; }

"$KAMA" pkg install tests/kama.json >/dev/null
for mode in --debug --release; do
    "$KAMA" build tests/kama.json --target WASM $mode -o "$tmp/tests$mode.js"
    "$NODE" "$tmp/tests$mode.js"
done
echo "test-wasm.sh: OK"
