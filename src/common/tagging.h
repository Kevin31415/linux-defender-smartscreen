#ifndef SMARTSCREEN_TAGGING_H
#define SMARTSCREEN_TAGGING_H

#include <string>

namespace smartscreen {
namespace tagging {

// 标签标记字符
constexpr const char* TAG_MARKER = "@";

// 检查文件名是否已包含标签标记
bool isTagged(const std::string& filename);

// 给文件名添加标签标记
// 规则1: 普通文件 - 在最后一个"."前插入"@"
// 规则2: 点文件 - 在第一个"."后插入"@"
// 如果已含标记或无扩展名则在末尾添加"@."
std::string tagFilename(const std::string& filename);

// 移除文件名中的标签标记
// 返回移除标记后的原始文件名
std::string untagFilename(const std::string& taggedFilename);

// 对路径中的文件名部分执行打标签操作
// 返回新的完整路径
std::string tagPath(const std::string& filepath);

// 对路径中的文件名部分执行移除标签操作
// 返回新的完整路径
std::string untagPath(const std::string& taggedFilepath);

// 仅返回移除标签后的文件名（不操作文件系统）
std::string untagName(const std::string& taggedFilepath);

} // namespace tagging
} // namespace smartscreen

#endif // SMARTSCREEN_TAGGING_H
