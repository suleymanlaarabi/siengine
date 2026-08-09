#!/usr/bin/env bash
set -euo pipefail

web_prefix="${SIENGINE_WEB_PREFIX:-$HOME/.local/siengine-emscripten}"
web_root="${SIENGINE_WEB_ROOT:-$(pwd)}"
web_output="${SIENGINE_WEB_OUTPUT:-$web_root/bin/emscripten-release}"

has_compile=false
for arg in "$@"; do
    if [ "$arg" = "-c" ]; then
        has_compile=true
        break
    fi
done

if [ "$has_compile" = true ]; then
    exec em++ -I"$web_prefix/include" "$@"
fi

exec em++ \
    -sUSE_WEBGL2=1 -sFULL_ES3=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -I"$web_prefix/include" -L"$web_prefix/lib" \
    -L"$web_output" \
    -L"$(dirname "$web_root")/siecs/bin/$(basename "$web_output")" \
    -L"$HOME/bake/src/sicore/bin/$(basename "$web_output")" \
    -L"$HOME/bake/src/sireflect/bin/$(basename "$web_output")" \
    -L"$HOME/bake/src/sijson/bin/$(basename "$web_output")" \
    "$@" --embed-file "$web_root/assets@/assets" \
    -lSDL3_image -lpng16 -lz -lSDL3
