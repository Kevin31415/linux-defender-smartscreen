#include "tagging.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace smartscreen {
namespace tagging {

bool isTagged(const std::string& filename) {
    return filename.find(TAG_MARKER) != std::string::npos;
}

std::string tagFilename(const std::string& filename) {
    if (isTagged(filename)) {
        return filename;
    }

    // 点文件: 以"."开头，在第一个"."后插入"@."
    if (!filename.empty() && filename[0] == '.') {
        return "." + std::string(TAG_MARKER) + filename.substr(1);
    }

    // 普通文件: 在最后一个"."前插入"@"
    auto dotPos = filename.rfind('.');
    if (dotPos != std::string::npos) {
        return filename.substr(0, dotPos) + TAG_MARKER + filename.substr(dotPos);
    }

    // 无扩展名: 在末尾添加"@."
    return filename + TAG_MARKER + ".";
}

std::string untagFilename(const std::string& taggedFilename) {
    if (!isTagged(taggedFilename)) {
        return taggedFilename;
    }

    // 点文件: ".@.xxx" -> ".xxx"
    if (taggedFilename.size() >= 3 &&
        taggedFilename[0] == '.' &&
        taggedFilename[1] == '@' &&
        taggedFilename[2] == '.') {
        return "." + taggedFilename.substr(3);
    }

    // 普通文件: 在"@"处拆分并重组
    auto atPos = taggedFilename.rfind(TAG_MARKER);
    if (atPos != std::string::npos) {
        return taggedFilename.substr(0, atPos) + taggedFilename.substr(atPos + 1);
    }

    return taggedFilename;
}

std::string tagPath(const std::string& filepath) {
    fs::path p(filepath);
    std::string filename = p.filename().string();

    if (fs::is_directory(p)) {
        return filepath;
    }

    std::string taggedName = tagFilename(filename);
    return (p.parent_path() / taggedName).string();
}

std::string untagPath(const std::string& taggedFilepath) {
    fs::path p(taggedFilepath);
    std::string filename = p.filename().string();
    std::string originalName = untagFilename(filename);
    return (p.parent_path() / originalName).string();
}

std::string untagName(const std::string& taggedFilepath) {
    fs::path p(taggedFilepath);
    std::string filename = p.filename().string();
    std::string originalName = untagFilename(filename);
    return (p.parent_path() / originalName).string();
}

} // namespace tagging
} // namespace smartscreen
