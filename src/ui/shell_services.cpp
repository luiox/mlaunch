#include "shell_services.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <ShObjIdl.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <atlbase.h>

#include <utility>

#include "utils/string_util.h"

namespace core {

bool ShellLaunchExecutor::Launch(const std::string& target_path, const std::string& arguments, std::string* error) {
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";

    std::wstring target_w = launcher::util::Utf8ToWide(target_path);
    std::wstring args_w = launcher::util::Utf8ToWide(arguments);
    sei.lpFile = target_w.c_str();
    sei.lpParameters = args_w.empty() ? nullptr : args_w.c_str();
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei)) {
        if (error != nullptr) {
            *error = "launch failed";
        }
        return false;
    }

    if (sei.hProcess) {
        CloseHandle(sei.hProcess);
    }
    return true;
}

std::optional<std::pair<std::string, std::string>> ShellShortcutResolver::Resolve(const std::string& shortcut_path) {
    CComPtr<IShellLinkW> shell_link;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&shell_link)))) {
        return std::nullopt;
    }

    CComPtr<IPersistFile> persist_file;
    if (FAILED(shell_link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persist_file)))) {
        return std::nullopt;
    }

    std::wstring link_w = launcher::util::Utf8ToWide(shortcut_path);
    if (FAILED(persist_file->Load(link_w.c_str(), STGM_READ))) {
        return std::nullopt;
    }

    wchar_t target[MAX_PATH] = {0};
    wchar_t args[2048] = {0};
    WIN32_FIND_DATAW fd{};

    if (FAILED(shell_link->GetPath(target, MAX_PATH, &fd, SLGP_RAWPATH))) {
        return std::nullopt;
    }
    shell_link->GetArguments(args, 2048);

    std::wstring target_w(target);
    if (target_w.empty()) {
        return std::nullopt;
    }

    return std::make_pair(launcher::util::WideToUtf8(target_w), launcher::util::WideToUtf8(args));
}

std::wstring PickOpenPath(HWND owner_window, const wchar_t* filter) {
    wchar_t file_path[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner_window;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) {
        return {};
    }
    return file_path;
}

} // namespace core
