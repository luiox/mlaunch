#pragma once

#include <Windows.h>

#include <optional>
#include <string>
#include <utility>

#include "launcher_core.h"

namespace core {

/** @brief Launches targets via ShellExecuteExW (UI-side default executor). */
class ShellLaunchExecutor : public LaunchExecutor {
public:
    bool Launch(const std::string& target_path, const std::string& arguments,
                const std::string& working_dir, std::string* error) override;
};

/** @brief Resolves .lnk shortcuts via IShellLinkW (UI-side default resolver). */
class ShellShortcutResolver : public ShortcutResolver {
public:
    std::optional<std::pair<std::string, std::string>> Resolve(const std::string& shortcut_path) override;
};

/** @brief Modal open-file dialog; returns wide path or empty on cancel. */
std::wstring PickOpenPath(HWND owner_window, const wchar_t* filter);

/** @brief Modal folder-picker dialog; returns wide path or empty on cancel. */
std::wstring PickFolderPath(HWND owner_window);

} // namespace core
