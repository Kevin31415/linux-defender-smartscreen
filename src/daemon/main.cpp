#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <csignal>
#include <pthread.h>

#include "../common/tagging.h"
#include "../common/logger.h"
#include "watcher.h"

namespace fs = std::filesystem;

static smartscreen::daemon::Watcher* g_watcher = nullptr;

static void signalHandler(int sig) {
    if (g_watcher) {
        g_watcher->stop();
    }
}

static std::string getDownloadsDir() {
    const char* home = std::getenv("HOME");
    if (!home) {
        return "";
    }
    return std::string(home) + "/Downloads";
}

static void printUsage() {
    std::cout << "用法:" << std::endl;
    std::cout << "  smartscreend                  启动守护进程（监控 ~/Downloads）" << std::endl;
    std::cout << "  smartscreend --check <file>   检查文件是否有标签" << std::endl;
    std::cout << "  smartscreend --tag <file>     手动为文件打标签" << std::endl;
    std::cout << "  smartscreend --untag <file>   移除文件标签（重命名）" << std::endl;
    std::cout << "  smartscreend --untag-name <file>  输出移除标签后的文件名（不重命名）" << std::endl;
    std::cout << "  smartscreend --help           显示帮助信息" << std::endl;
}

static int cmdCheck(const std::string& filepath) {
    fs::path p(filepath);
    std::string filename = p.filename().string();
    bool tagged = smartscreen::tagging::isTagged(filename);
    smartscreen::logger::logCheck(filepath, tagged);
    return tagged ? 0 : 1;
}

static int cmdTag(const std::string& filepath) {
    fs::path p(filepath);
    if (!fs::exists(p)) {
        smartscreen::logger::logWarn("文件不存在: " + filepath);
        return 1;
    }
    if (fs::is_directory(p)) {
        smartscreen::logger::logWarn("跳过目录: " + filepath);
        return 1;
    }

    std::string filename = p.filename().string();
    if (smartscreen::tagging::isTagged(filename)) {
        smartscreen::logger::logInfo("文件已有标签: " + filepath);
        return 0;
    }

    // 只标记有执行权限的文件
    std::error_code permEc;
    auto perms = fs::status(p, permEc).permissions();
    if (permEc || (perms & fs::perms::owner_exec) == fs::perms::none) {
        smartscreen::logger::logWarn("文件无执行权限，跳过: " + filepath);
        return 1;
    }

    std::string taggedPath = smartscreen::tagging::tagPath(filepath);
    std::error_code ec;
    fs::rename(p, taggedPath, ec);
    if (ec) {
        smartscreen::logger::logWarn("重命名失败: " + filepath + " - " + ec.message());
        return 1;
    }

    smartscreen::logger::logTag(filepath, taggedPath);
    return 0;
}

static int cmdUntag(const std::string& filepath) {
    fs::path p(filepath);
    if (!fs::exists(p)) {
        smartscreen::logger::logWarn("文件不存在: " + filepath);
        return 1;
    }

    std::string filename = p.filename().string();
    if (!smartscreen::tagging::isTagged(filename)) {
        smartscreen::logger::logInfo("文件无标签: " + filepath);
        return 0;
    }

    std::string originalPath = smartscreen::tagging::untagPath(filepath);
    std::error_code ec;
    fs::rename(p, originalPath, ec);
    if (ec) {
        smartscreen::logger::logWarn("重命名失败: " + filepath + " - " + ec.message());
        return 1;
    }

    smartscreen::logger::logUntag(filepath, originalPath);
    return 0;
}

static int cmdUntagName(const std::string& filepath) {
    std::string originalName = smartscreen::tagging::untagName(filepath);
    std::cout << originalName << std::endl;
    return 0;
}

static int cmdDaemon() {
    std::string downloadsDir = getDownloadsDir();
    if (downloadsDir.empty()) {
        smartscreen::logger::logWarn("无法获取 HOME 环境变量");
        return 1;
    }

    if (!fs::exists(downloadsDir) || !fs::is_directory(downloadsDir)) {
        smartscreen::logger::logWarn("Downloads 目录不存在: " + downloadsDir);
        return 1;
    }

    smartscreen::logger::logInfo("smartscreend 启动");
    smartscreen::logger::logInfo("监控目录: " + downloadsDir);

    smartscreen::daemon::Watcher watcher(downloadsDir);
    g_watcher = &watcher;

    // 设置信号处理
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    watcher.start([&downloadsDir](const std::string& filepath) {
        fs::path p(filepath);
        std::string filename = p.filename().string();

        // 跳过已标记的文件
        if (smartscreen::tagging::isTagged(filename)) {
            return;
        }

        // 只标记有执行权限的文件
        std::error_code permEc;
        auto perms = fs::status(p, permEc).permissions();
        if (permEc || (perms & fs::perms::owner_exec) == fs::perms::none) {
            return;
        }

        // 打标签
        std::string taggedPath = smartscreen::tagging::tagPath(filepath);
        std::error_code ec;
        fs::rename(p, taggedPath, ec);
        if (ec) {
            smartscreen::logger::logWarn("标签失败: " + filepath + " - " + ec.message());
            return;
        }

        smartscreen::logger::logTag(filepath, taggedPath);
    });

    // 等待信号
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);

    int sig;
    sigwait(&mask, &sig);

    g_watcher = nullptr;
    watcher.stop();
    smartscreen::logger::logInfo("smartscreend 已退出");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return cmdDaemon();
    }

    std::string arg = argv[1];

    if (arg == "--help" || arg == "-h") {
        printUsage();
        return 0;
    }

    if (arg == "--check" && argc >= 3) {
        return cmdCheck(argv[2]);
    }

    if (arg == "--tag" && argc >= 3) {
        return cmdTag(argv[2]);
    }

    if (arg == "--untag" && argc >= 3) {
        return cmdUntag(argv[2]);
    }

    if (arg == "--untag-name" && argc >= 3) {
        return cmdUntagName(argv[2]);
    }

    printUsage();
    return 1;
}
