#!/bin/sh
set -eu

example="${1:-}"
case "$example" in
    c)
        port=8080
        ;;
    cpp)
        port=8081
        ;;
    *)
        echo "usage: $0 <c|cpp>" >&2
        exit 2
        ;;
esac

repo_root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
web_target="${SIENGINE_WEB_TARGET:-x64-Linux}"
web_output="$repo_root/examples/$example/bin/${web_target}-release"

"$repo_root/tools/build_web.sh" "$example"

echo "siengine: serving $web_output"
echo "siengine: opening http://127.0.0.1:$port"
(
    sleep 1
    xdg-open "http://127.0.0.1:$port" >/dev/null 2>&1 || true
) &

exec python3 -m http.server "$port" --bind 127.0.0.1 --directory "$web_output"
