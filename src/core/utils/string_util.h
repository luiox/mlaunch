#pragma once

#include <string>

namespace launcher::util {

std::wstring Utf8ToWide(const std::string& text);
std::string WideToUtf8(const std::wstring& text);

/** @brief 去除首尾 ASCII 空白；全空白返回空串。 */
std::string Trim(const std::string& text);

} // namespace launcher::util
