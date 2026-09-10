// Base64 编解码：libca crypto 的 base64 实现复用（用户指定走 libca）。

#include "tool_registry.h"

#include "libca/crypto/base64.hpp"

#include "tool_util.h"

namespace tools {
namespace {

using internal::CopyLine;
using internal::SplitCommand;
using internal::TextLine;

ToolOutput Run(const std::string& raw_args) {
    ToolOutput out;
    std::string mode;
    std::string text;
    SplitCommand(raw_args, &mode, &text);

    if (text.empty()) {
        out.error = "用法：b64 e <文本> 编码 / b64 d <base64> 解码";
        return out;
    }

    if (mode == "e" || mode == "encode") {
        const auto encoded = ca::crypto::base64_encode(
            ca::core::ByteSlice(reinterpret_cast<const ca::u8*>(text.data()), text.size()));
        out.lines.push_back(CopyLine("编码", encoded));
        out.primary = encoded;
    } else if (mode == "d" || mode == "decode") {
        const auto result = ca::crypto::base64_decode(text);
        if (result.is_err()) {
            out.error =
                std::string("base64 解码失败：") + ca::crypto::to_string(result.unwrap_err());
            return out;
        }
        // libca Bytes 是引用计数共享存储，拷出到连续 string 供展示/复制。
        ca::core::Bytes bytes = result.unwrap();
        std::string decoded(bytes.len(), '\0');
        if (!decoded.empty()) {
            bytes.copy_to_slice(reinterpret_cast<ca::u8*>(&decoded[0]), decoded.size());
        }
        // 二进制内容直接显示会打乱列表，预览行给出字节摘要，复制仍是原文。
        bool printable = !decoded.empty();
        for (unsigned char ch : decoded) {
            if (ch < 0x20 && ch != '\t' && ch != '\n' && ch != '\r') {
                printable = false;
                break;
            }
        }
        if (printable) {
            out.lines.push_back(CopyLine("解码", decoded));
        } else {
            // 二进制内容直接显示会打乱列表，预览行只给字节摘要，复制仍是原文。
            ToolLine raw;
            raw.text =
                "解码  <" + std::to_string(decoded.size()) + " 字节二进制数据，回车复制原文>";
            raw.selectable = true;
            raw.copy_text = decoded;
            out.lines.push_back(raw);
        }
        out.primary = decoded;
    } else {
        out.error = "未知子命令：" + mode + "（用法：b64 e <文本> / b64 d <base64>）";
        return out;
    }

    out.ok = true;
    return out;
}

} // namespace

ToolDef BuildBase64Tool() {
    ToolDef def;
    def.keyword = "b64";
    def.name = "Base64 编解码";
    def.usage = "b64 e <文本> 编码 / b64 d <base64> 解码";
    def.run = &Run;
    return def;
}

} // namespace tools
