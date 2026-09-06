// 哈希摘要：MD5 / SHA1 / SHA256，复用 libca crypto（返回小写 hex）。
// hash <algo> <text> 与 md5/sha1/sha256 快捷关键字共享同一实现。

#include "tool_registry.h"

#include "libca/crypto/md5.hpp"
#include "libca/crypto/sha1.hpp"
#include "libca/crypto/sha256.hpp"

#include "tool_util.h"

namespace tools {
namespace {

using internal::CopyLine;
using internal::SplitCommand;

enum class Algorithm { kMd5, kSha1, kSha256 };

const char* AlgorithmName(Algorithm algo) {
    switch (algo) {
    case Algorithm::kMd5: return "MD5";
    case Algorithm::kSha1: return "SHA1";
    case Algorithm::kSha256: return "SHA256";
    }
    return "?";
}

std::string Digest(Algorithm algo, const std::string& text) {
    switch (algo) {
    case Algorithm::kMd5: return ca::crypto::MD5()(text);
    case Algorithm::kSha1: return ca::crypto::SHA1()(text);
    case Algorithm::kSha256: return ca::crypto::SHA256()(text);
    }
    return "";
}

ToolOutput RunWith(Algorithm fixed_algo, const std::string& raw_args) {
    ToolOutput out;

    // 快捷关键字（md5 <text>）直接定算法；hash 需要首词指定算法。
    Algorithm algo = fixed_algo;
    std::string text = internal::Trim(raw_args);
    if (fixed_algo == Algorithm::kMd5) {
        // 仅当首词恰好是算法名时才按"hash 风格"解析，避免把正文 md5 误吞。
        // 普通文本照常整体哈希：md5 hello → MD5("hello")。
        std::string head;
        std::string tail;
        SplitCommand(raw_args, &head, &tail);
        if (head == "md5" || head == "sha1" || head == "sha256") {
            algo = head == "md5" ? Algorithm::kMd5 : (head == "sha1" ? Algorithm::kSha1 : Algorithm::kSha256);
            text = tail;
        }
    }

    if (text.empty()) {
        out.error = "用法：输入待哈希文本";
        return out;
    }

    const std::string hex = Digest(algo, text);
    out.lines.push_back(CopyLine(AlgorithmName(algo), hex));
    out.primary = hex;
    out.ok = true;
    return out;
}

ToolOutput RunHash(const std::string& raw_args) {
    ToolOutput out;
    std::string head;
    std::string text;
    SplitCommand(raw_args, &head, &text);

    Algorithm algo = Algorithm::kMd5;
    if (head == "md5") {
        algo = Algorithm::kMd5;
    } else if (head == "sha1") {
        algo = Algorithm::kSha1;
    } else if (head == "sha256") {
        algo = Algorithm::kSha256;
    } else {
        out.error = "用法：hash <md5|sha1|sha256> <文本>";
        return out;
    }

    if (text.empty()) {
        out.error = "用法：hash <md5|sha1|sha256> <文本>";
        return out;
    }

    const std::string hex = Digest(algo, text);
    out.lines.push_back(CopyLine(AlgorithmName(algo), hex));
    out.primary = hex;
    out.ok = true;
    return out;
}

} // namespace

ToolDef BuildHashTool() {
    ToolDef def;
    def.keyword = "hash";
    def.name = "哈希计算";
    def.usage = "hash <md5|sha1|sha256> <文本>";
    def.run = &RunHash;
    return def;
}

ToolDef BuildMd5Tool() {
    ToolDef def;
    def.keyword = "md5";
    def.name = "MD5 摘要";
    def.usage = "md5 <文本>";
    def.run = [](const std::string& args) { return RunWith(Algorithm::kMd5, args); };
    return def;
}

ToolDef BuildSha1Tool() {
    ToolDef def;
    def.keyword = "sha1";
    def.name = "SHA1 摘要";
    def.usage = "sha1 <文本>";
    def.run = [](const std::string& args) { return RunWith(Algorithm::kSha1, args); };
    return def;
}

ToolDef BuildSha256Tool() {
    ToolDef def;
    def.keyword = "sha256";
    def.name = "SHA256 摘要";
    def.usage = "sha256 <文本>";
    def.run = [](const std::string& args) { return RunWith(Algorithm::kSha256, args); };
    return def;
}

} // namespace tools
