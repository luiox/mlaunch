#pragma once

#include <string>

namespace launcher::log {

void Info(const std::string& message);
void Warn(const std::string& message);
void Error(const std::string& message);
void Debug(const std::string& message);

} // namespace launcher::log
