#!/bin/sh
set -e
cd "$(dirname "$0")"

HOOK_PATH="$(pwd)/build/libsmartscreen_hook.so"

add_to_rc() {
    rc="$1"
    if [ ! -f "$rc" ]; then return; fi
    if grep -q "LD_PRELOAD=.*libsmartscreen_hook" "$rc" 2>/dev/null; then
        echo "$rc 已配置，跳过"
        return
    fi
    echo "" >> "$rc"
    echo "# Linux Defender SmartScreen" >> "$rc"
    echo "export LD_PRELOAD=$HOOK_PATH" >> "$rc"
    echo "已写入 $rc"
}

add_to_rc "$HOME/.bashrc"
add_to_rc "$HOME/.zshrc"
add_to_rc "$HOME/.profile"
add_to_rc "$HOME/.xprofile"
