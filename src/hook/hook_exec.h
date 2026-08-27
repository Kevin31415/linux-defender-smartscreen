#ifndef SMARTSCREEN_HOOK_EXEC_H
#define SMARTSCREEN_HOOK_EXEC_H

#ifdef __cplusplus
extern "C" {
#endif

// 检查文件名是否带标签 (内联快速检查，避免依赖外部库)
int hook_is_tagged(const char* filename);

// 从带标签的文件名恢复原名
// 返回原名写入 buf，失败返回 NULL
const char* hook_untag_name(const char* taggedName, char* buf, int bufsize);

// 调用 smartscreen-dialog 弹窗
// 返回 0=允许, 1=取消, -1=错误
int hook_launch_dialog(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif // SMARTSCREEN_HOOK_EXEC_H
