#!/bin/bash
#
# Linux Defender SmartScreen - Shell Wrapper
#
# 使用方法: 将以下行添加到 ~/.bashrc 或 ~/.zshrc:
#   source /path/to/smartscreen.sh
#

# 找到 smartscreend 和 smartscreen-dialog 的路径
SMARTSCREEN_BIN_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../build"
SMARTSCREEN_DAEMON="${SMARTSCREEN_BIN_DIR}/smartscreend"
SMARTSCREEN_DIALOG="${SMARTSCREEN_BIN_DIR}/smartscreen-dialog"

# 如果在 PATH 中能找到则使用
if ! command -v smartscreend &>/dev/null; then
    if [ -x "${SMARTSCREEN_DAEMON}" ]; then
        PATH="${SMARTSCREEN_BIN_DIR}:${PATH}"
    fi
fi

# 包装函数: 在执行前检查标签
smartscreen_exec() {
    local cmd="$1"
    shift

    # 只处理存在的普通文件
    if [ -f "$cmd" ] && smartscreend --check "$cmd" 2>/dev/null; then
        # 文件有标签，弹窗询问
        smartscreen-dialog --file "$cmd"
        if [ $? -eq 0 ]; then
            # 用户允许: 移除标签并执行
            local original
            original=$(smartscreend --untag-name "$cmd")
            if [ "$cmd" != "$original" ]; then
                mv -- "$cmd" "$original"
            fi
            exec "$original" "$@"
        else
            echo "操作已取消"
            return 1
        fi
    fi

    # 无标签或非文件，直接执行
    exec "$cmd" "$@"
}

# 覆盖 command_not_found_handle 以捕获直接执行
# 注意: 这只在用户直接输入命令名时触发
# 对于 ./program 形式，需要使用 alias 或函数
alias exec='smartscreen_exec' 2>/dev/null
