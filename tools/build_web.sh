#!/bin/sh
set -eu

if ! command -v emcc >/dev/null || ! command -v em++ >/dev/null; then
    echo "siengine: make web requires the Emscripten SDK (emcc and em++)" >&2
    exit 1
fi

web_prefix="${SIENGINE_WEB_PREFIX:-$HOME/.local/siengine-emscripten}"
if [ ! -f "$web_prefix/include/SDL3_image/SDL_image.h" ] || \
   [ ! -f "$web_prefix/lib/libSDL3_image.a" ] || \
   [ ! -f "$web_prefix/lib/libSDL3.a" ]; then
    echo "siengine: SDL3 web dependencies are missing in $web_prefix" >&2
    exit 1
fi

repo_root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
web_target="${SIENGINE_WEB_TARGET:-x64-Linux}"
web_config="${web_target}-release"
export SIENGINE_WEB_PREFIX="$web_prefix"
export SIENGINE_WEB_ROOT="$repo_root"
export SIENGINE_WEB_OUTPUT="$repo_root/bin/$web_config"

build_example() {
    example="$1"
    web_output="$repo_root/$example/bin/$web_config"

    if [ "$example" = "examples/cpp" ]; then
        export SIENGINE_WEB_CXX_LINK=1
    else
        export SIENGINE_WEB_CXX_LINK=0
    fi

    echo "siengine: building $example for web..."
    bake rebuild "$example" --target "$web_target" \
        --cc "$repo_root/tools/emcc_web.sh" --cxx "$repo_root/tools/em++_web.sh" \
        --static $recursive --cfg release

    install -m 644 "$repo_root/$example/index.html" "$web_output/index.html"
    echo "siengine: web build ready in $web_output"
    recursive=
}

recursive=-r
case "${1:-all}" in
    all)
        build_example examples/c
        build_example examples/cpp
        ;;
    c)
        build_example examples/c
        ;;
    cpp)
        build_example examples/cpp
        ;;
    *)
        echo "usage: $0 [all|c|cpp]" >&2
        exit 2
        ;;
esac
