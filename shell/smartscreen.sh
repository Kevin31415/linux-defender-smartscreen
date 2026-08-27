#!/bin/bash
#
# Linux Defender SmartScreen - Shell Wrapper
#
# 使用方法: 将以下行添加到 ~/.bashrc 或 ~/.zshrc:
#   source /path/to/smartscreen.sh
#

# 自动定位二进制目录
SMARTSCREEN_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SMARTSCREEN_BIN_DIR="${SMARTSCREEN_SCRIPT_DIR}/../build"

# 如果 build 目录不存在，尝试从 PATH 查找
if [ ! -x "${SMARTSCREEN_BIN_DIR}/smartscreend" ]; then
    if command -v smartscreend &>/dev/null; then
        SMARTSCREEN_BIN_DIR="$(dirname "$(command -v smartscreend)")"
    fi
fi

SMARTSCREEN_DAEMON="${SMARTSCREEN_BIN_DIR}/smartscreend"
SMARTSCREEN_DIALOG="${SMARTSCREEN_BIN_DIR}/smartscreen-dialog"

# 检查二进制是否存在
if [ ! -x "${SMARTSCREEN_DAEMON}" ]; then
    echo "[SmartScreen] 警告: smartscreend 未找到，功能不可用" >&2
    return 1 2>/dev/null || exit 1
fi

# 核心包装函数: 检查标签并执行
ss-run() {
    if [ $# -eq 0 ]; then
        echo "用法: ss-run <命令> [参数...]" >&2
        return 1
    fi

    local cmd="$1"
    shift

    # 只处理存在的普通文件
    if [ -f "$cmd" ] && "${SMARTSCREEN_DAEMON}" --check "$cmd" >/dev/null 2>&1; then
        # 文件有标签，弹窗询问
        "${SMARTSCREEN_DIALOG}" --file "$cmd"
        local dialog_result=$?

        if [ $dialog_result -eq 0 ]; then
            # 用户允许: 移除标签并执行
            local original
            original=$("${SMARTSCREEN_DAEMON}" --untag-name "$cmd")
            if [ -n "$original" ] && [ "$cmd" != "$original" ]; then
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

# 自动打标签: 手动为文件打标签
ss-tag() {
    if [ $# -eq 0 ]; then
        echo "用法: ss-tag <文件...>" >&2
        return 1
    fi

    local exit_code=0
    for file in "$@"; do
        "${SMARTSCREEN_DAEMON}" --tag "$file"
        if [ $? -ne 0 ]; then
            exit_code=1
        fi
    done
    return $exit_code
}

# 自动移除标签: 手动移除文件标签
ss-untag() {
    if [ $# -eq 0 ]; then
        echo "用法: ss-untag <文件...>" >&2
        return 1
    fi

    local exit_code=0
    for file in "$@"; do
        "${SMARTSCREEN_DAEMON}" --untag "$file"
        if [ $? -ne 0 ]; then
            exit_code=1
        fi
    done
    return $exit_code
}

# 检查标签
ss-check() {
    if [ $# -eq 0 ]; then
        echo "用法: ss-check <文件...>" >&2
        return 1
    fi

    for file in "$@"; do
        "${SMARTSCREEN_DAEMON}" --check "$file"
    done
}

# 启动守护进程
ss-daemon() {
    "${SMARTSCREEN_DAEMON}" "$@"
}

echo "[SmartScreen] 已加载。命令: ss-run, ss-tag, ss-untag, ss-check, ss-daemon"
