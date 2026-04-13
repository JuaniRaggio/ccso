#!/bin/bash

LINE1="// This is an open source non-commercial project. Dear PVS-Studio, please check it."
LINE2="// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com"

add() {
    find . -name '*.c' -not -path '*/vendor/*' | while read -r f; do
        if ! head -1 "$f" | grep -q "PVS-Studio"; then
            sed -i "1i\\${LINE1}\n${LINE2}" "$f"
        fi
    done
}

remove() {
    find . -name '*.c' -not -path '*/vendor/*' -exec sed -i '/^\/\/ .*PVS-Studio/d' {} \;
}

case "${1:-}" in
    add)    add ;;
    remove) remove ;;
    *)      echo "Usage: $0 {add|remove}" >&2; exit 1 ;;
esac
