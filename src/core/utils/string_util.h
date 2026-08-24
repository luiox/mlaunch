#pragma once

#include <string>

namespace launcher::util {

std::wstring Utf8ToWide(const std::string& text);
std::string WideToUtf8(const std::wstring& text);

} // namespace launcher::util
