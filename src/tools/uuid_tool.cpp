// UUID v4 生成：复用 libca uuid（系统 CSPRNG 随机源）。

#include "tool_registry.h"

#include "libca/uuid/uuid.hpp"

#include <cctype>

#include "tool_util.h"

namespace tools {
namespace {

using internal::TextLine;

ToolOutput Run(const std::string& raw_args) {
    ToolOutput out;

    // 空参数生成 1 个；数字 n 批量生成（钳到 1..50，防止手滑输 100000 卡 UI）。
    int count = 1;
    std::string args = internal::Trim(raw_args);
    if (!args.empty()) {
        bool all_digits = true;
        for (char ch : args) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                all_digits = false;
                break;
            }
        }
        if (!all_digits) {
            out.error = "用法：uuid [数量 1-50]";
            return out;
        }
        count = std::stoi(args);
        if (count < 1) {
            count = 1;
        }
        if (count > 50) {
            count = 50;
        }
    }

    for (int i = 0; i < count; ++i) {
        const std::string id = ca::uuid::v4();
        ToolLine line;
        line.text = id;
        line.selectable = true;
        line.copy_text = id;
        out.lines.push_back(line);
        if (out.primary.empty()) {
            out.primary = id;
        }
    }
    if (count > 1) {
        out.lines.push_back(internal::TextLine("共 " + std::to_string(count) + " 个，均不重复"));
    }

    out.ok = true;
    return out;
}

} // namespace

ToolDef BuildUuidTool() {
    ToolDef def;
    def.keyword = "uuid";
    def.name = "UUID 生成";
    def.usage = "uuid [数量]，生成随机 UUID v4";
    def.run = &Run;
    return def;
}

} // namespace tools
