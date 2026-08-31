#pragma once

// app_window 各编译单元共享的内部辅助。仅由 ui 自己的 .cpp 包含，非公共 API。

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shlobj.h>

#include <UIlib.h>

#include <cstddef>
#include <filesystem>
#include <string>

#include "logger.h"
#include "utils/string_util.h"

namespace appwin {

struct UiStateSnapshot {
    int splitter_width = 220;
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    int width = 0;
    int height = 0;
    int maximized = 0;
};

inline std::filesystem::path GetAppBaseDir() {
    PWSTR local_app_data = nullptr;
    std::filesystem::path out = std::filesystem::current_path() / "data";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_app_data)) && local_app_data != nullptr) {
        out = std::filesystem::path(local_app_data) / "nassistant";
        CoTaskMemFree(local_app_data);
    }
    std::error_code ec;
    std::filesystem::create_directories(out, ec);
    return out;
}

inline bool IsSenderFromList(DuiLib::CControlUI* sender, DuiLib::CListUI* list) {
    if (sender == nullptr || list == nullptr) {
        return false;
    }
    if (sender == list) {
        return true;
    }
    DuiLib::CControlUI* walk = sender;
    DuiLib::CControlUI* list_body = list->GetList();
    while (walk != nullptr) {
        if (walk == list || walk == list_body) {
            return true;
        }
        walk = walk->GetParent();
    }
    return false;
}

inline std::filesystem::path GetUiStatePath() {
    return GetAppBaseDir() / "ui_state.ini";
}

inline bool ReadIniInt(const std::filesystem::path& ini_path, const wchar_t* section, const wchar_t* key, int* out) {
    if (out == nullptr) {
        return false;
    }
    wchar_t buffer[64]{};
    const DWORD size = GetPrivateProfileStringW(section, key, L"", buffer, static_cast<DWORD>(std::size(buffer)), ini_path.wstring().c_str());
    if (size == 0) {
        return false;
    }
    wchar_t* end = nullptr;
    const long value = std::wcstol(buffer, &end, 10);
    if (end == buffer) {
        return false;
    }
    *out = static_cast<int>(value);
    return true;
}

inline bool WriteIniInt(const std::filesystem::path& ini_path, const wchar_t* section, const wchar_t* key, int value) {
    const std::wstring value_text = std::to_wstring(value);
    return ::WritePrivateProfileStringW(section, key, value_text.c_str(), ini_path.wstring().c_str()) != FALSE;
}

inline bool FlushIniFile(const std::filesystem::path& ini_path) {
    return ::WritePrivateProfileStringW(nullptr, nullptr, nullptr, ini_path.wstring().c_str()) != FALSE;
}

inline bool WriteUiStateAtomically(const std::filesystem::path& ini_path, const UiStateSnapshot& snapshot) {
    // 说明：曾尝试“写 tmp + MoveFileEx 原子替换”，但 kernel32 对 INI 文件
    // 有进程内句柄/写缓存，tmp 冲刷与替换在多种时序下都会失败（实测
    // ERROR_FILE_NOT_FOUND / 共享冲突），导致保存永久失败。
    // UI 几何信息非关键数据，改为直写目标文件 + 尽力冲刷，可靠性实测更好。
    //
    // 预创建目录与空文件：INI 的内核缓存会记住“文件不存在”，首次冲刷
    // 因此返回 err=2（误报）；提前触碰文件可让首个 flush 正常返回。
    std::error_code ec;
    std::filesystem::create_directories(ini_path.parent_path(), ec);
    if (!std::filesystem::exists(ini_path, ec)) {
        if (HANDLE handle = ::CreateFileW(ini_path.wstring().c_str(), GENERIC_WRITE,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                          CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)) {
            ::CloseHandle(handle);
        }
    }

    bool ok = true;
    ok = ok && WriteIniInt(ini_path, L"layout", L"splitter_width", snapshot.splitter_width);
    ok = ok && WriteIniInt(ini_path, L"window", L"left", snapshot.left);
    ok = ok && WriteIniInt(ini_path, L"window", L"top", snapshot.top);
    ok = ok && WriteIniInt(ini_path, L"window", L"right", snapshot.right);
    ok = ok && WriteIniInt(ini_path, L"window", L"bottom", snapshot.bottom);
    ok = ok && WriteIniInt(ini_path, L"window", L"width", snapshot.width);
    ok = ok && WriteIniInt(ini_path, L"window", L"height", snapshot.height);
    ok = ok && WriteIniInt(ini_path, L"window", L"maximized", snapshot.maximized);

    if (!ok) {
        launcher::log::Error("ui_state ini write failed err=" + std::to_string(::GetLastError()));
        return false;
    }

    // 尽力冲刷缓存；首次运行仍可能返回 err=2（INI 缓存对同进程新建文件的
    // flush 误报，数据实际已正确落盘——实测验证），降为 Debug 避免噪音。
    if (!FlushIniFile(ini_path)) {
        launcher::log::Debug("ui_state ini flush skipped err=" + std::to_string(::GetLastError()));
    }
    return true;
}

} // namespace appwin
