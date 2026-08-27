#include "tagging.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace smartscreen {
namespace tagging {

bool isTagged(const std::string& filename) {
    if (filename.empty()) return false;

    // 点文件: 检查 ".@." 模式
    // ".@.bashrc" → tagged
    if (filename.size() >= 3 &&
        filename[0] == '.' &&
        filename[1] == '@' &&
        filename[2] == '.') {
        return true;
    }

    // 普通文件: "@" 在最后一个 "." 之前
    auto atPos = filename.find('@');
    if (atPos == std::string::npos || atPos == 0) return false;

    auto lastDot = filename.rfind('.');
    if (lastDot == std::string::npos) {
        // 无扩展名: 检查 "@." 结尾
        return filename.size() >= 2 &&
               filename[filename.size() - 2] == '@' &&
               filename[filename.size() - 1] == '.';
    }

    return atPos < lastDot;
}

std::string tagFilename(const std::string& filename) {
    if (filename.empty()) return filename;

    if (isTagged(filename)) {
        return filename;
    }

    // 点文件: 在 "." 后插入 "@."
    // ".hidden" → ".@.hidden"
    if (filename[0] == '.') {
        return filename.substr(0, 1) + "@." + filename.substr(1);
    }

    // 普通文件: 在最后一个"."前插入"@"
    auto dotPos = filename.rfind('.');
    if (dotPos != std::string::npos) {
        // "archive.tar.gz" → "archive.tar@.gz"
        return filename.substr(0, dotPos) + TAG_MARKER + filename.substr(dotPos);
    }

    // 无扩展名: 在末尾添加"@."
    return filename + TAG_MARKER + ".";
}

std::string untagFilename(const std::string& taggedFilename) {
    if (taggedFilename.empty()) return taggedFilename;

    if (!isTagged(taggedFilename)) {
        return taggedFilename;
    }

    // 点文件: ".@.xxx" → ".xxx" (移除位置1的 "@")
    if (taggedFilename.size() >= 3 &&
        taggedFilename[0] == '.' &&
        taggedFilename[1] == '@' &&
        taggedFilename[2] == '.') {
        return "." + taggedFilename.substr(3);
    }

    // 无扩展名: "xxx@." → "xxx" (移除末尾 "@.")
    if (taggedFilename.size() >= 2 &&
        taggedFilename[taggedFilename.size() - 2] == '@' &&
        taggedFilename[taggedFilename.size() - 1] == '.') {
        return taggedFilename.substr(0, taggedFilename.size() - 2);
    }

    // 普通文件: "xxx@.yyy" → "xxx.yyy" (移除 "@" 及其后的 ".")
    auto atPos = taggedFilename.find(TAG_MARKER);
    if (atPos != std::string::npos && atPos + 1 < taggedFilename.size() &&
        taggedFilename[atPos + 1] == '.') {
        return taggedFilename.substr(0, atPos) + taggedFilename.substr(atPos + 1);
    }

    return taggedFilename;
}

std::string tagPath(const std::string& filepath) {
    fs::path p(filepath);
    std::string filename = p.filename().string();

    std::string taggedName = tagFilename(filename);
    if (taggedName == filename) {
        return filepath;
    }

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
