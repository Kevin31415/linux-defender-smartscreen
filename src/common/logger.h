#ifndef SMARTSCREEN_LOGGER_H
#define SMARTSCREEN_LOGGER_H

#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace smartscreen {
namespace logger {

enum class Level {
    TAG,
    UNTAG,
    CHECK,
    WARN,
    INFO
};

// 获取当前时间戳字符串 [YYYY-MM-DD HH:MM:SS]
std::string timestamp();

// 格式化并输出日志到 stdout
void log(Level level, const std::string& message);

// 便捷函数
void logTag(const std::string& from, const std::string& to);
void logUntag(const std::string& from, const std::string& to);
void logCheck(const std::string& file, bool tagged);
void logWarn(const std::string& message);
void logInfo(const std::string& message);

} // namespace logger
} // namespace smartscreen

#endif // SMARTSCREEN_LOGGER_H
