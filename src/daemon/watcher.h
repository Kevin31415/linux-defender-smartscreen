#ifndef SMARTSCREEN_WATCHER_H
#define SMARTSCREEN_WATCHER_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <map>
#include <mutex>

namespace smartscreen {
namespace daemon {

class Watcher {
public:
    using FileCallback = std::function<void(const std::string& filepath)>;

    explicit Watcher(const std::string& watchDir);
    ~Watcher();

    Watcher(const Watcher&) = delete;
    Watcher& operator=(const Watcher&) = delete;

    void start(FileCallback onNewFile);
    void stop();
    bool isRunning() const;

private:
    void addWatchRecursive(const std::string& dir);
    void addWatch(const std::string& dir);

    std::string watchDir_;
    std::atomic<bool> running_{false};
    std::thread watchThread_;
    int inotifyFd_{-1};

    std::mutex watchMutex_;
    std::map<int, std::string> wdToPath_;  // watch descriptor → directory path
};

} // namespace daemon
} // namespace smartscreen

#endif // SMARTSCREEN_WATCHER_H
