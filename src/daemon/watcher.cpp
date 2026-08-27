#include "watcher.h"
#include "../common/logger.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace smartscreen {
namespace daemon {

Watcher::Watcher(const std::string& watchDir)
    : watchDir_(watchDir) {}

Watcher::~Watcher() {
    stop();
}

void Watcher::addWatch(const std::string& dir) {
    uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE_SELF;
    int wd = inotify_add_watch(inotifyFd_, dir.c_str(), mask);
    if (wd >= 0) {
        std::lock_guard<std::mutex> lock(watchMutex_);
        wdToPath_[wd] = dir;
    } else {
        logger::logWarn("无法监控: " + dir + " - " + strerror(errno));
    }
}

void Watcher::addWatchRecursive(const std::string& dir) {
    addWatch(dir);

    std::error_code ec;
    for (auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
        if (entry.is_directory(ec)) {
            addWatchRecursive(entry.path().string());
        }
    }
}

void Watcher::start(FileCallback onNewFile) {
    if (running_.load()) {
        return;
    }

    inotifyFd_ = inotify_init1(IN_NONBLOCK);
    if (inotifyFd_ < 0) {
        logger::logWarn("inotify_init1 失败: " + std::string(strerror(errno)));
        return;
    }

    // 递归添加所有子目录的 watch
    addWatchRecursive(watchDir_);
    logger::logInfo("开始监控目录: " + watchDir_ + " (递归)");

    running_.store(true);

    watchThread_ = std::thread([this, onNewFile]() {
        constexpr size_t BUF_SIZE = 8192;
        alignas(inotify_event) char buf[BUF_SIZE];

        while (running_.load()) {
            ssize_t len = read(inotifyFd_, buf, BUF_SIZE);
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                logger::logWarn("inotify read 失败: " + std::string(strerror(errno)));
                break;
            }

            for (char* ptr = buf; ptr < buf + len; ) {
                auto* event = reinterpret_cast<inotify_event*>(ptr);

                std::string dirPath;
                {
                    std::lock_guard<std::mutex> lock(watchMutex_);
                    auto it = wdToPath_.find(event->wd);
                    if (it != wdToPath_.end()) {
                        dirPath = it->second;
                    }
                }

                if (!dirPath.empty() && event->len > 0) {
                    std::string fullPath = dirPath + "/" + event->name;

                    // 新建子目录: 添加 watch
                    if (event->mask & IN_CREATE && (event->mask & IN_ISDIR)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        addWatchRecursive(fullPath);
                        logger::logInfo("新增监控子目录: " + fullPath);
                    }

                    // 目录被删除: 清理 watch 映射
                    if (event->mask & IN_DELETE_SELF) {
                        std::lock_guard<std::mutex> lock(watchMutex_);
                        wdToPath_.erase(event->wd);
                    }

                    // 文件事件: 处理打标签
                    if (!(event->mask & IN_ISDIR) &&
                        (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO))) {
                        std::error_code ec;
                        if (fs::is_regular_file(fullPath, ec) && !ec) {
                            onNewFile(fullPath);
                        }
                    }
                }

                ptr += sizeof(inotify_event) + event->len;
            }
        }

        // 清理所有 watch
        std::lock_guard<std::mutex> lock(watchMutex_);
        for (auto& [wd, path] : wdToPath_) {
            inotify_rm_watch(inotifyFd_, wd);
        }
        wdToPath_.clear();
    });
}

void Watcher::stop() {
    running_.store(false);
    if (watchThread_.joinable()) {
        watchThread_.join();
    }
    if (inotifyFd_ >= 0) {
        close(inotifyFd_);
        inotifyFd_ = -1;
    }
    logger::logInfo("监控已停止");
}

bool Watcher::isRunning() const {
    return running_.load();
}

} // namespace daemon
} // namespace smartscreen
