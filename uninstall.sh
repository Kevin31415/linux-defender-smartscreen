#!/bin/sh
set -e

remove_from_rc() {
    rc="$1"
    if [ ! -f "$rc" ]; then return; fi
    if ! grep -q "LD_PRELOAD=.*libsmartscreen_hook" "$rc" 2>/dev/null; then
        echo "$rc 未配置，跳过"
        return
    fi
    sed -i '/# Linux Defender SmartScreen/d' "$rc"
    sed -i '/export LD_PRELOAD=.*libsmartscreen_hook/d' "$rc"
    echo "已移除 $rc"
}

remove_from_rc "$HOME/.bashrc"
remove_from_rc "$HOME/.zshenv"
remove_from_rc "$HOME/.profile"
remove_from_rc "$HOME/.xprofile"

echo ""
echo "卸载完成。请执行以下命令生效："
echo "  source ~/.bashrc"
echo "  source ~/.zshenv"
