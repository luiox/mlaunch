#include "string_util.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>

namespace launcher::util {

std::string Trim(const std::string& text) {
    const auto begin = std::find_if_not(text.begin(), text.end(),
                                        [](unsigned char ch) { return std::isspace(ch); });
    const auto end = std::find_if_not(text.rbegin(), text.rend(),
                                      [](unsigned char ch) { return std::isspace(ch); }).base();
    return (begin < end) ? std::string(begin, end) : std::string();
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) {
        return std::wstring(text.begin(), text.end());
    }
    std::wstring out(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size);
    return out;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return std::string(text.begin(), text.end());
    }
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size, nullptr, nullptr);
    return out;
}

} // namespace launcher::util
