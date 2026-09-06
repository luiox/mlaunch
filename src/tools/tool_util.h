#pragma once

// 工具实现内部共用的小函数。不对外暴露（CLI 与 UI 只依赖 tool_registry.h）。

// windows.h 的 min/max 宏会打坏 std::min/std::max/std::clamp。
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cctype>
#include <string>

#include "tool_registry.h"

namespace tools::internal {

// UTF-8 → UTF-16。工具需要打开文件时用（std::ifstream 接受宽 path）。
inline std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int size = ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                           static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                          wide.data(), size);
    return wide;
}

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
