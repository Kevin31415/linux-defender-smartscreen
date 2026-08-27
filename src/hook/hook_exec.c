#define _GNU_SOURCE
#include "hook_exec.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/syscall.h>
#include <dlfcn.h>

extern char** environ;

static __thread int in_hook = 0;

static const char* find_dialog(void) {
    static char path[PATH_MAX];

    Dl_info info;
    if (dladdr((void*)find_dialog, &info) && info.dli_fname) {
        const char* so_path = info.dli_fname;
        const char* slash = strrchr(so_path, '/');
        if (slash) {
            int dir_len = slash - so_path;
            snprintf(path, sizeof(path), "%.*s/smartscreen-dialog", dir_len, so_path);
            if (access(path, X_OK) == 0) return path;
        }
    }

    const char* e = getenv("SMARTSCREEN_DIALOG");
    if (e && access(e, X_OK) == 0) return e;
    return NULL;
}

int hook_is_tagged(const char* f) {
    if (!f) return 0;
    const char* at = strchr(f, '@');
    if (!at) return 0;
    if (*(at+1)=='.') return 1;
    if (*(at+1)=='\0' && at>f && *(at-1)!='/') return 1;
    if (at==f+1 && f[0]=='.' && *(at+1)=='.') return 1;
    return 0;
}

static void approve(const char* tagged, char* out, int size) {
    const char* base = strrchr(tagged, '/');
    base = base ? base + 1 : tagged;
    int prefix = base - tagged;

    // ".@.bashrc" → ".#.bashrc"
    if (base[0]=='.' && base[1]=='@' && base[2]=='.') {
        snprintf(out, size, "%.*s.#.%s", prefix, tagged, base+3); return;
    }
    int blen = strlen(base);
    // "file@." → "file#."
    if (blen>=2 && base[blen-2]=='@' && base[blen-1]=='.') {
        snprintf(out, size, "%.*s%.*s", prefix, tagged, blen-2, base);
        out[prefix+blen-2]='#'; out[prefix+blen-1]='.'; out[prefix+blen]='\0';
        return;
    }
    // "file@.txt" → "file#.txt"
    const char* at = strchr(base, '@');
    if (at && *(at+1)=='.') {
        int before = at - base;
        snprintf(out, size, "%.*s#.%s", prefix+before, tagged, at+2);
        return;
    }
    snprintf(out, size, "%s", tagged);
}

static int show_dialog(const char* path) {
    const char* dlg = find_dialog();
    if (!dlg) return -1;
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) { execl(dlg, "smartscreen-dialog", "--file", path, (char*)NULL); _exit(127); }
    int st;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    return -1;
}

/*
 * 拦截逻辑:
 * 返回值通过 out_newpath 输出重命名后的路径:
 *   NULL  → 无拦截，用原 filename
 *   非NULL → 已重命名，调用者必须用 out_newpath 执行
 * 返回: 0=放行, -1=阻止
 */
static int do_intercept(const char* filename, char** out_newpath) {
    *out_newpath = NULL;
    if (in_hook) return 0;
    if (!filename || !hook_is_tagged(filename)) return 0;

    char resolved[PATH_MAX];
    if (!realpath(filename, resolved))
        snprintf(resolved, sizeof(resolved), "%s", filename);

    in_hook = 1;
    int r = show_dialog(resolved);
    in_hook = 0;

    if (r != 0) return -1;

    char orig[PATH_MAX];
    approve(resolved, orig, sizeof(orig));
    if (rename(resolved, orig) == 0) {
        *out_newpath = strdup(orig);
    }
    return 0;
}

int execve(const char* fn, char* const argv[], char* const envp[]) {
    char* newp = NULL;
    int ret = do_intercept(fn, &newp);
    if (ret == -1) { free(newp); return -1; }
    const char* exec_fn = newp ? newp : fn;
    int r = syscall(SYS_execve, exec_fn, argv, envp);
    free(newp);
    return r;
}

int execv(const char* fn, char* const argv[]) {
    char* newp = NULL;
    int ret = do_intercept(fn, &newp);
    if (ret == -1) { free(newp); return -1; }
    const char* exec_fn = newp ? newp : fn;
    int r = syscall(SYS_execve, exec_fn, argv, environ);
    free(newp);
    return r;
}

int execvp(const char* fn, char* const argv[]) {
    char* newp = NULL;
    int ret = do_intercept(fn, &newp);
    if (ret == -1) { free(newp); return -1; }
    const char* exec_fn = newp ? newp : fn;
    int r = syscall(SYS_execve, exec_fn, argv, environ);
    free(newp);
    return r;
}

int execvpe(const char* fn, char* const argv[], char* const envp[]) {
    char* newp = NULL;
    int ret = do_intercept(fn, &newp);
    if (ret == -1) { free(newp); return -1; }
    const char* exec_fn = newp ? newp : fn;
    int r = syscall(SYS_execve, exec_fn, argv, envp);
    free(newp);
    return r;
}
