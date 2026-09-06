// URL 百分号编解码。编码保留 RFC 3986 unreserved（字母数字 -_.~），
// 其余字节转 %XX（UTF-8 字节级，天然支持中文）；解码只认 %XX，"+" 保留原样。

#include "tool_registry.h"

#include <cstdio>
#include <string>

#include "tool_util.h"

namespace tools {
namespace {

using internal::CopyLine;
using internal::SplitCommand;

bool IsUnreserved(unsigned char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
           (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~';
}

std::string PercentEncode(const std::string& text) {
    static const char* kHex = "0123456789ABCDEF";
    std::string out;
    out.reserve(text.size() * 3);
    for (unsigned char ch : text) {
        if (IsUnreserved(ch)) {
            out += static_cast<char>(ch);
        } else {
            out += '%';
            out += kHex[ch >> 4];
            out += kHex[ch & 15];
        }
    }
    return out;
}

bool HexValue(char ch, int* out_value) {
    if (ch >= '0' && ch <= '9') { *out_value = ch - '0'; return true; }
    if (ch >= 'a' && ch <= 'f') { *out_value = ch - 'a' + 10; return true; }
    if (ch >= 'A' && ch <= 'F') { *out_value = ch - 'A' + 10; return true; }
    return false;
}

std::string PercentDecode(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size()) {
            int hi = 0;
            int lo = 0;
            if (HexValue(text[i + 1], &hi) && HexValue(text[i + 2], &lo)) {
                out += static_cast<char>(hi * 16 + lo);
                i += 2;
                continue;
            }
        }
        out += text[i];
    }
    return out;
}

ToolOutput Run(const std::string& raw_args) {
    ToolOutput out;
    std::string mode;
    std::string text;
    SplitCommand(raw_args, &mode, &text);

    if (text.empty()) {
        out.error = "用法：url e <文本> 编码 / url d <已编码文本> 解码";
        return out;
    }

    if (mode == "e" || mode == "encode") {
        const std::string encoded = PercentEncode(text);
        out.lines.push_back(CopyLine("编码", encoded));
        out.primary = encoded;
    } else if (mode == "d" || mode == "decode") {
        const std::string decoded = PercentDecode(text);
        out.lines.push_back(CopyLine("解码", decoded));
        out.primary = decoded;
    } else {
        out.error = "未知子命令：" + mode + "（用法：url e <文本> / url d <已编码文本>）";
        return out;
    }

    out.ok = true;
    return out;
}

} // namespace

ToolDef BuildUrlTool() {
    ToolDef def;
    def.keyword = "url";
    def.name = "URL 编解码";
    def.usage = "url e <文本> 编码 / url d <已编码文本> 解码";
    def.run = &Run;
    return def;
}

} // namespace tools
