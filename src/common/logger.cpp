#include "logger.h"

namespace smartscreen {
namespace logger {

std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void log(Level level, const std::string& message) {
    const char* levelStr = "";
    switch (level) {
        case Level::TAG:   levelStr = "TAG";   break;
        case Level::UNTAG: levelStr = "UNTAG"; break;
        case Level::CHECK: levelStr = "CHECK"; break;
        case Level::WARN:  levelStr = "WARN";  break;
        case Level::INFO:  levelStr = "INFO";  break;
    }

    std::cout << "[" << timestamp() << "] [" << levelStr << "] " << message << std::endl;
}

void logTag(const std::string& from, const std::string& to) {
    log(Level::TAG, from + " → " + to);
}

void logUntag(const std::string& from, const std::string& to) {
    log(Level::UNTAG, from + " → " + to);
}

void logCheck(const std::string& file, bool tagged) {
    log(Level::CHECK, file + " → " + (tagged ? "tagged" : "untagged"));
}

void logWarn(const std::string& message) {
    log(Level::WARN, message);
}

void logInfo(const std::string& message) {
    log(Level::INFO, message);
}

} // namespace logger
} // namespace smartscreen
