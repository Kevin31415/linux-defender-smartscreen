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

void Watcher::start(FileCallback onNewFile) {
    if (running_.load()) {
        return;
    }

    inotifyFd_ = inotify_init1(IN_NONBLOCK);
    if (inotifyFd_ < 0) {
        logger::logWarn("inotify_init1 失败: " + std::string(strerror(errno)));
        return;
    }

    uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE;
    int wd = inotify_add_watch(inotifyFd_, watchDir_.c_str(), mask);
    if (wd < 0) {
        logger::logWarn("inotify_add_watch 失败: " + watchDir_ + " - " + strerror(errno));
        close(inotifyFd_);
        inotifyFd_ = -1;
        return;
    }

    running_.store(true);
    logger::logInfo("开始监控目录: " + watchDir_);

    watchThread_ = std::thread([this, onNewFile, wd]() {
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

                // 跳过目录事件
                if (event->mask & IN_ISDIR) {
                    ptr += sizeof(inotify_event) + event->len;
                    continue;
                }

                // 只处理文件事件
                if (event->len > 0 &&
                    (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE))) {
                    std::string filepath = watchDir_ + "/" + event->name;

                    // 等待文件写入完成（特别是 IN_CREATE 后可能还有写入）
                    if (event->mask & IN_CREATE) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }

                    // 验证文件存在且是常规文件
                    std::error_code ec;
                    if (fs::is_regular_file(filepath, ec) && !ec) {
                        onNewFile(filepath);
                    }
                }

                ptr += sizeof(inotify_event) + event->len;
            }
        }

        inotify_rm_watch(inotifyFd_, wd);
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
