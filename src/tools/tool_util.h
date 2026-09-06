#pragma once

// 工具实现内部共用的小函数。不对外暴露（CLI 与 UI 只依赖 tool_registry.h）。

#include <cctype>
#include <string>

#include "tool_registry.h"

namespace tools::internal {

inline std::string Trim(const std::string& s) {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(begin, end - begin);
}

// 按第一个空白切分；head 转小写（子命令不区分大小写），tail 保留原样（文本内容不能动）。
inline void SplitCommand(const std::string& input, std::string* head, std::string* tail) {
    const std::string trimmed = Trim(input);
    const std::size_t space = trimmed.find_first_of(" \t");
    if (space == std::string::npos) {
        *head = trimmed;
        tail->clear();
        return;
    }
    *head = trimmed.substr(0, space);
    *tail = Trim(trimmed.substr(space + 1));
    for (char& ch : *head) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
}

inline ToolLine TextLine(std::string text) {
    ToolLine line;
    line.text = std::move(text);
    return line;
}

inline ToolLine CopyLine(std::string label, std::string value) {
    ToolLine line;
    line.text = label + "  " + value;
    line.selectable = true;
    line.copy_text = std::move(value);
    return line;
}

} // namespace tools::internal
