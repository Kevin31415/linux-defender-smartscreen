#define _GNU_SOURCE
#include "hook_exec.h"
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>

// 原始 exec 函数指针
typedef int (*execve_fn)(const char*, char* const[], char* const[]);
typedef int (*execv_fn)(const char*, char* const[]);
typedef int (*execvp_fn)(const char*, char* const[]);
typedef int (*execvpe_fn)(const char*, char* const[], char* const[]);

static execve_fn real_execve = NULL;
static execv_fn real_execv = NULL;
static execvp_fn real_execvp = NULL;
static execvpe_fn real_execvpe = NULL;

// 防止递归调用
static __thread int in_hook = 0;

// 查找 smartscreen-dialog 的路径
static const char* find_dialog_path(void) {
    static char path[PATH_MAX];

    // 优先: 同目录下的 smartscreen-dialog
    char self_dir[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", self_dir, sizeof(self_dir) - 1);
    if (len > 0) {
        self_dir[len] = '\0';
        char* slash = strrchr(self_dir, '/');
        if (slash) {
            *slash = '\0';
            snprintf(path, sizeof(path), "%s/smartscreen-dialog", self_dir);
            if (access(path, X_OK) == 0) return path;
        }
    }

    // 回退: 从环境变量
    const char* env = getenv("SMARTSCREEN_DIALOG");
    if (env && access(env, X_OK) == 0) return env;

    // 回退: PATH 中查找
    const char* pathenv = getenv("PATH");
    if (pathenv) {
        char* pathcopy = strdup(pathenv);
        char* dir = strtok(pathcopy, ":");
        while (dir) {
            snprintf(path, sizeof(path), "%s/smartscreen-dialog", dir);
            if (access(path, X_OK) == 0) {
                free(pathcopy);
                return path;
            }
            dir = strtok(NULL, ":");
        }
        free(pathcopy);
    }

    return NULL;
}

// 快速检查文件名是否含 '@' 标记
int hook_is_tagged(const char* filename) {
    if (!filename) return 0;
    const char* at = strchr(filename, '@');
    if (!at) return 0;
    // 后面必须跟着 '.' (普通文件) 或者是末尾的 "@." (无扩展名)
    if (*(at + 1) == '.') return 1;
    if (*(at + 1) == '\0' && at > filename && *(at - 1) != '/') return 1;
    // 点文件: .@.xxx
    if (at == filename + 1 && filename[0] == '.' && *(at + 1) == '.') return 1;
    return 0;
}

// 从带标签的文件名恢复原名
const char* hook_untag_name(const char* taggedName, char* buf, int bufsize) {
    if (!taggedName || !buf) return NULL;

    const char* base = strrchr(taggedName, '/');
    base = base ? base + 1 : taggedName;

    if (!hook_is_tagged(base)) {
        snprintf(buf, bufsize, "%s", taggedName);
        return buf;
    }

    // 点文件: .@.xxx → .xxx
    if (base[0] == '.' && base[1] == '@' && base[2] == '.') {
        int prefix_len = base - taggedName;
        int rest_len = strlen(base + 3);
        if (prefix_len + 1 + rest_len >= bufsize) return NULL;
        memcpy(buf, taggedName, prefix_len);
        buf[prefix_len] = '.';
        memcpy(buf + prefix_len + 1, base + 3, rest_len + 1);
        return buf;
    }

    // 无扩展名: xxx@. → xxx
    int blen = strlen(base);
    if (blen >= 2 && base[blen-2] == '@' && base[blen-1] == '.') {
        int prefix_len = base - taggedName;
        int name_len = blen - 2;
        if (prefix_len + name_len >= bufsize) return NULL;
        memcpy(buf, taggedName, prefix_len + name_len);
        buf[prefix_len + name_len] = '\0';
        return buf;
    }

    // 普通文件: xxx@.yyy → xxx.yyy
    const char* at = strchr(base, '@');
    if (at && *(at + 1) == '.') {
        int prefix_len = base - taggedName;
        int before_at = at - base;
        const char* after_dot = at + 2;
        int after_len = strlen(after_dot);
        if (prefix_len + before_at + 1 + after_len >= bufsize) return NULL;
        memcpy(buf, taggedName, prefix_len + before_at);
        buf[prefix_len + before_at] = '.';
        memcpy(buf + prefix_len + before_at + 1, after_dot, after_len + 1);
        return buf;
    }

    snprintf(buf, bufsize, "%s", taggedName);
    return buf;
}

// 调用 smartscreen-dialog 弹窗
int hook_launch_dialog(const char* filepath) {
    const char* dialog = find_dialog_path();
    if (!dialog) return -1;

    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        // 子进程: 执行 smartscreen-dialog
        execl(dialog, "smartscreen-dialog", "--file", filepath, (char*)NULL);
        _exit(127);
    }

    // 父进程: 等待
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

// 核心拦截逻辑
static int hook_check_and_intercept(const char* filename) {
    if (in_hook) return 0;
    if (!filename) return 0;

    // 只处理带标签的文件
    if (!hook_is_tagged(filename)) return 0;

    // 获取绝对路径
    char resolved[PATH_MAX];
    if (realpath(filename, resolved) == NULL) {
        // 文件不存在，不拦截
        return 0;
    }

    in_hook = 1;

    // 弹窗询问
    int result = hook_launch_dialog(resolved);

    if (result == 0) {
        // 用户允许: 移除标签并重命名
        char original[PATH_MAX];
        if (hook_untag_name(resolved, original, sizeof(original))) {
            rename(resolved, original);
        }
        in_hook = 0;
        return 0; // 继续执行
    }

    in_hook = 0;
    return 1; // 阻止执行
}

// ===== Hook execve =====
int execve(const char* filename, char* const argv[], char* const envp[]) {
    if (!real_execve) {
        real_execve = (execve_fn)dlsym(RTLD_NEXT, "execve");
    }

    if (hook_check_and_intercept(filename)) {
        return -1;
    }

    return real_execve(filename, argv, envp);
}

// ===== Hook execv =====
int execv(const char* filename, char* const argv[]) {
    if (!real_execv) {
        real_execv = (execv_fn)dlsym(RTLD_NEXT, "execv");
    }

    if (hook_check_and_intercept(filename)) {
        return -1;
    }

    return real_execv(filename, argv);
}

// ===== Hook execvp =====
int execvp(const char* filename, char* const argv[]) {
    if (!real_execvp) {
        real_execvp = (execvp_fn)dlsym(RTLD_NEXT, "execvp");
    }

    if (hook_check_and_intercept(filename)) {
        return -1;
    }

    return real_execvp(filename, argv);
}

// ===== Hook execvpe =====
int execvpe(const char* filename, char* const argv[], char* const envp[]) {
    if (!real_execvpe) {
        real_execvpe = (execvpe_fn)dlsym(RTLD_NEXT, "execvpe");
    }

    if (hook_check_and_intercept(filename)) {
        return -1;
    }

    return real_execvpe(filename, argv, envp);
}

// ===== Hook system() 中的 exec =====
// system() 内部调用 execve，已被上面的 hook 拦截

// ===== 构造/析构 =====
__attribute__((constructor))
static void hook_init(void) {
    real_execve = (execve_fn)dlsym(RTLD_NEXT, "execve");
    real_execv = (execv_fn)dlsym(RTLD_NEXT, "execv");
    real_execvp = (execvp_fn)dlsym(RTLD_NEXT, "execvp");
    real_execvpe = (execvpe_fn)dlsym(RTLD_NEXT, "execvpe");
}

__attribute__((destructor))
static void hook_fini(void) {
}
