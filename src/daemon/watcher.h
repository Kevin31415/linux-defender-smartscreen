#ifndef SMARTSCREEN_WATCHER_H
#define SMARTSCREEN_WATCHER_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>

namespace smartscreen {
namespace daemon {

class Watcher {
public:
    using FileCallback = std::function<void(const std::string& filepath)>;

    explicit Watcher(const std::string& watchDir);
    ~Watcher();

    // 禁止拷贝
    Watcher(const Watcher&) = delete;
    Watcher& operator=(const Watcher&) = delete;

    // 启动监控（阻塞直到 stop 被调用）
    void start(FileCallback onNewFile);

    // 停止监控
    void stop();

    // 检查是否正在运行
    bool isRunning() const;

private:
    std::string watchDir_;
    std::atomic<bool> running_{false};
    std::thread watchThread_;
    int inotifyFd_{-1};
};

} // namespace daemon
} // namespace smartscreen

#endif // SMARTSCREEN_WATCHER_H
