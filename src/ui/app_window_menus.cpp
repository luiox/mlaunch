#include "app_window.h"

#include <Windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>

#include "app_window_internal.h"
#include "constants.h"
#include "list_controller.h"
#include "logger.h"
#include "search_controller.h"
#include "utils/string_util.h"

using namespace DuiLib;

void AppWindow::ShowBackupRecoveryMenu() {
    const auto backups = backend_.ListBackups();
    if (backups.empty()) {
        status_.Warn("数据损坏且无可用备份");
        return;
    }

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, L"数据文件已损坏，请选择备份恢复：");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    UINT added = 0;
    for (const auto& backup : backups) {
        if (added >= launcher::constants::command::kBackupRestoreMax) {
            break;
        }
        AppendMenuW(menu, MF_STRING,
            launcher::constants::command::kBackupRestoreBase + added,
            launcher::util::Utf8ToWide(backup.name).c_str());
        ++added;
    }

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kBackupStartFresh, L"使用全新数据启动");

    RECT rc{};
    ::GetWindowRect(m_hWnd, &rc);
    POINT menu_point{(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    SetForegroundWindow(m_hWnd);
    const UINT command_id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, menu_point.x, menu_point.y, 0, m_hWnd, nullptr);
    DestroyMenu(menu);

    if (command_id != 0) {
        ExecuteBackupCommand(command_id);
    }
}

void AppWindow::ExecuteBackupCommand(UINT command_id) {
    if (command_id == launcher::constants::command::kBackupStartFresh) {
        status_.Warn("已保留全新数据；原备份仍在 backups/ 目录");
        return;
    }
    if (command_id < launcher::constants::command::kBackupRestoreBase) {
        return;
    }

    const auto index = static_cast<int>(command_id - launcher::constants::command::kBackupRestoreBase);
    const auto backups = backend_.ListBackups();
    if (index < 0 || index >= static_cast<int>(backups.size())) {
        status_.Error("备份项已不可用");
        return;
    }

    std::string error;
    if (!backend_.RestoreFromBackup(backups[index].path, &error)) {
        status_.Error("恢复失败：" + error);
        return;
    }

    RenderGroups();
    if (!group_ids_.empty()) {
        SelectGroupByIndex(0);
    }
    status_.Info("已从 " + backups[index].name + " 恢复");
}

void AppWindow::ShowGroupContextMenu(const POINT& screen_point) {
    if (IsActiveGroupRecycleBin()) {
        status_.Info("回收站由系统自动管理");
        return;
    }

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kGroupAdd, L"新建分组");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kGroupRename, L"重命名分组");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kGroupDelete, L"删除分组");

    SetForegroundWindow(m_hWnd);
    const UINT command_id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen_point.x, screen_point.y, 0, m_hWnd, nullptr);
    DestroyMenu(menu);

    if (command_id != 0) {
        ExecuteGroupCommand(command_id);
    }
}

void AppWindow::ShowItemContextMenu(const POINT& screen_point) {
    HMENU menu = CreatePopupMenu();
    HMENU move_menu = nullptr;

    if (IsActiveGroupRecycleBin()) {
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemDelete, L"彻底删除");
    } else {
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemRunAs, L"以管理员身份运行");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemOpenFolder, L"打开所在位置");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemShellMenu, L"资源管理器菜单");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemCopyPath, L"复制完整路径");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemAdd, L"添加项目");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemEdit, L"编辑项目");
        const core::LaunchItem* selected_item = FindSelectedItem();
        const bool item_enabled = selected_item == nullptr || selected_item->enabled;
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemToggleEnabled,
                    item_enabled ? L"禁用条目" : L"启用条目");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemDelete, L"删除项目");

        move_menu = CreatePopupMenu();
        for (int i = 0; i < static_cast<int>(group_ids_.size()); ++i) {
            if (group_ids_[i] == active_group_id_) {
                continue;
            }
            const core::Group* group = nullptr;
            for (const auto& candidate : backend_.Data().groups) {
                if (candidate.id == group_ids_[i]) {
                    group = &candidate;
                    break;
                }
            }
            if (group != nullptr && !group->hidden) {
                AppendMenuW(move_menu, MF_STRING, launcher::constants::command::kItemMoveBase + static_cast<UINT>(i), launcher::util::Utf8ToWide(group->name).c_str());
            }
        }
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(move_menu), L"移动到分组");
    }

    SetForegroundWindow(m_hWnd);
    const UINT command_id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen_point.x, screen_point.y, 0, m_hWnd, nullptr);
    DestroyMenu(menu);

    if (command_id != 0) {
        ExecuteItemCommand(command_id);
    }
}

void AppWindow::ShowMainContextMenu(const POINT& screen_point, bool right_align) {
    HMENU menu = CreatePopupMenu();
    HMENU new_menu = CreatePopupMenu();

    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewCustom, L"自定义项目…");
    AppendMenuW(new_menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewEmpty, L"空项目");
    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewComputer, L"计算机");
    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewControlPanel, L"控制面板");
    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewRecycleBin, L"回收站");
    AppendMenuW(new_menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewLogoff, L"注销");
    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewShutdown, L"关机");
    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewReboot, L"重启");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(new_menu), L"新建项目");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainSortByName, L"按名称排序");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainSortByCount, L"按使用频率排序");
    AppendMenuW(menu, MF_STRING | (layout_locked_ ? MF_CHECKED : 0), launcher::constants::command::kMainToggleLock, L"锁定");
    AppendMenuW(menu, MF_STRING | (auto_hide_ ? MF_CHECKED : 0), launcher::constants::command::kMainToggleAutoHide, L"自动隐藏");

    HMENU convert_menu = CreatePopupMenu();
    AppendMenuW(convert_menu, MF_STRING, launcher::constants::command::kMainConvertRelative, L"转为相对路径（便携模式）…");
    AppendMenuW(convert_menu, MF_STRING, launcher::constants::command::kMainConvertAbsolute, L"转为绝对路径…");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(convert_menu), L"路径转换");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainImportData, L"导入数据");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainExportData, L"导出数据");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainSettings, L"设置");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainExit, L"退出");

    SetForegroundWindow(m_hWnd);
    const UINT flags = TPM_RETURNCMD | TPM_RIGHTBUTTON | (right_align ? TPM_RIGHTALIGN : 0);
    const UINT command_id = TrackPopupMenu(menu, flags, screen_point.x, screen_point.y, 0, m_hWnd, nullptr);
    DestroyMenu(menu);

    if (command_id != 0) {
        ExecuteMainCommand(command_id);
    }
}

void AppWindow::ExecuteMainCommand(UINT command_id) {
    switch (command_id) {
    case launcher::constants::command::kMainNewCustom:
        OpenItemDialog(false);
        return;
    case launcher::constants::command::kMainNewEmpty:
    case launcher::constants::command::kMainNewComputer:
    case launcher::constants::command::kMainNewControlPanel:
    case launcher::constants::command::kMainNewRecycleBin:
    case launcher::constants::command::kMainNewLogoff:
    case launcher::constants::command::kMainNewShutdown:
    case launcher::constants::command::kMainNewReboot:
        AddPresetSystemItem(command_id);
        return;
    case launcher::constants::command::kMainToggleLock:
        ToggleLayoutLock();
        return;
    case launcher::constants::command::kMainToggleAutoHide:
        ToggleAutoHide();
        return;
    case launcher::constants::command::kMainConvertRelative:
        ConvertItemPathsMenu(true);
        return;
    case launcher::constants::command::kMainConvertAbsolute:
        ConvertItemPathsMenu(false);
        return;
    case launcher::constants::command::kMainSortByName: {
        const core::Group* group = FindActiveGroup();
        if (group == nullptr) {
            status_.Warn("请先选择分组");
            return;
        }
        std::string error;
        if (!backend_.SortGroupItemsByName(group->id, &error)) {
            status_.Error("排序失败：" + error);
            return;
        }
        RenderItems();
        status_.Info("已按名称排序");
        return;
    }
    case launcher::constants::command::kMainSortByCount: {
        const core::Group* group = FindActiveGroup();
        if (group == nullptr) {
            status_.Warn("请先选择分组");
            return;
        }
        std::string error;
        if (!backend_.SortGroupItemsByLaunchCount(group->id, &error)) {
            status_.Error("排序失败：" + error);
            return;
        }
        RenderItems();
        status_.Info("已按使用频率排序");
        return;
    }
    case launcher::constants::command::kMainImportData: {
        const auto path = PickJsonFilePath();
        if (path.empty()) {
            return;
        }
        ImportPonerFile(std::filesystem::path(path));
        return;
    }
    case launcher::constants::command::kMainExportData: {
        const auto path = PickSaveJsonFilePath();
        if (path.empty()) {
            return;
        }
        std::string error;
        if (!backend_.ExportData(std::filesystem::path(path), &error)) {
            status_.Error("导出失败：" + error);
            return;
        }
        status_.Info("已导出到 " + launcher::util::WideToUtf8(std::filesystem::path(path).filename().wstring()));
        return;
    }
    case launcher::constants::command::kMainSettings:
        OpenSettingsDialog();
        return;
    case launcher::constants::command::kMainExit:
        ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
        return;
    default:
        return;
    }
}

void AppWindow::ExecuteGroupCommand(UINT command_id) {
    if (command_id == launcher::constants::command::kGroupAdd) {
        // 对齐 VB6 原版：新建分组不弹窗，按当前可见分组数+1 直接创建“分组N”。
        std::size_t visible_count = 0;
        for (const auto& group : backend_.Data().groups) {
            if (!group.hidden) {
                ++visible_count;
            }
        }
        std::string created_id;
        std::string error;
        std::string name;
        for (std::size_t suffix = visible_count + 1; suffix <= visible_count + 64; ++suffix) {
            name = "分组" + std::to_string(suffix);
            created_id = backend_.AddGroup(name, &error);
            if (!created_id.empty()) {
                break;
            }
            error.clear();
        }
        if (created_id.empty()) {
            status_.Error("新建分组失败：" + error);
            return;
        }
        active_group_id_ = created_id;
        RenderGroups();
        for (int i = 0; i < static_cast<int>(group_ids_.size()); ++i) {
            if (group_ids_[i] == created_id) {
                SelectGroupByIndex(i);
                break;
            }
        }
        status_.Info("已创建分组 " + name);
        return;
    }

    if (command_id == launcher::constants::command::kGroupRename) {
        const core::Group* group = FindActiveGroup();
        if (group == nullptr) {
            status_.Warn("请先选择分组");
            return;
        }
        OpenGroupDialog(true, group->id);
        return;
    }

    if (command_id == launcher::constants::command::kGroupDelete) {
        DeleteActiveGroup();
    }
}

void AppWindow::ExecuteItemCommand(UINT command_id) {
    if (command_id == launcher::constants::command::kItemRunAs) {
        RunSelectedItemAsAdmin();
        return;
    }
    if (command_id == launcher::constants::command::kItemOpenFolder) {
        OpenSelectedItemFolder();
        return;
    }
    if (command_id == launcher::constants::command::kItemShellMenu) {
        ShowSelectedItemShellMenu();
        return;
    }
    if (command_id == launcher::constants::command::kItemCopyPath) {
        CopySelectedItemPath();
        return;
    }
    if (command_id == launcher::constants::command::kItemAdd) {
        OpenItemDialog(false);
        return;
    }
    if (command_id == launcher::constants::command::kItemEdit) {
        OpenItemDialog(true);
        return;
    }
    if (command_id == launcher::constants::command::kItemToggleEnabled) {
        ToggleSelectedItemEnabled();
        return;
    }
    if (command_id == launcher::constants::command::kItemDelete) {
        DeleteSelectedItem();
        return;
    }
    if (command_id >= launcher::constants::command::kItemMoveBase) {
        const int group_index = static_cast<int>(command_id - launcher::constants::command::kItemMoveBase);
        if (group_index < 0 || group_index >= static_cast<int>(group_ids_.size())) {
            status_.Warn("目标分组无效");
            return;
        }
        MoveSelectedItemToGroup(group_ids_[group_index]);
    }
}

std::wstring AppWindow::PickJsonFilePath() const {
    wchar_t file_path[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"JSON 数据 (*.json)\0*.json\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) {
        return {};
    }
    return file_path;
}

std::wstring AppWindow::PickSaveJsonFilePath() const {
    wchar_t file_path[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"JSON 数据 (*.json)\0*.json\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrDefExt = L"json";
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetSaveFileNameW(&ofn)) {
        return {};
    }
    return file_path;
}

bool AppWindow::ImportPonerFile(const std::filesystem::path& path) {
    std::string error;
    const auto merged = backend_.ImportPonerData(path, &error);
    if (merged == 0 && !error.empty()) {
        status_.Error("导入失败：" + error);
        return false;
    }

    RenderGroups();
    if (!group_ids_.empty()) {
        SelectGroupByIndex(0);
    }
    RenderItems();

    if (merged == 0) {
        status_.Info("没有需要导入的新内容");
    } else {
        status_.Info("已导入 " + std::to_string(merged) + " 条（来源 " + path.filename().string() + "）");
    }
    return true;
}

bool AppWindow::DeleteActiveGroup() {
    const core::Group* active_group = FindActiveGroup();
    if (active_group == nullptr) {
        status_.Warn("请先选择分组");
        return false;
    }

    std::string target_group_id;
    for (const auto& group_id : group_ids_) {
        if (group_id != active_group->id) {
            target_group_id = group_id;
            break;
        }
    }
    if (target_group_id.empty()) {
        status_.Warn("至少保留一个分组");
        return false;
    }

    const int confirmed = MessageBoxW(m_hWnd,
        L"该分组将被删除，其中的条目会移入其他分组。确定删除？",
        L"删除分组",
        MB_ICONQUESTION | MB_YESNO);
    if (confirmed != IDYES) {
        status_.Warn("已取消删除");
        return false;
    }

    std::string error;
    if (!backend_.DeleteGroup(active_group->id, target_group_id, &error)) {
        status_.Error("删除分组失败：" + error);
        return false;
    }

    active_group_id_ = target_group_id;
    selected_item_id_.clear();
    selected_item_group_id_.clear();
    RenderGroups();
    for (int i = 0; i < static_cast<int>(group_ids_.size()); ++i) {
        if (group_ids_[i] == active_group_id_) {
            SelectGroupByIndex(i);
            break;
        }
    }
    status_.Info("分组已删除");
    return true;
}

bool AppWindow::RunSelectedItemAsAdmin() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("请先选择条目");
        return false;
    }
    if (item->item_type == "separator") {
        status_.Warn("分隔条目无法启动");
        return false;
    }

    const std::wstring target_w = launcher::util::Utf8ToWide(item->target_path);
    const std::wstring args_w = launcher::util::Utf8ToWide(item->arguments);
    HINSTANCE instance = ShellExecuteW(
        m_hWnd,
        L"runas",
        target_w.c_str(),
        args_w.empty() ? nullptr : args_w.c_str(),
        nullptr,
        SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(instance) <= 32) {
        status_.Error("管理员方式启动失败");
        return false;
    }

    status_.Info("已以管理员身份启动");
    return true;
}

bool AppWindow::OpenSelectedItemFolder() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("请先选择条目");
        return false;
    }
    if (item->target_path.empty()) {
        status_.Warn("目标路径为空");
        return false;
    }

    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(launcher::util::Utf8ToWide(item->target_path).c_str());
    if (pidl == nullptr) {
        status_.Error("打开所在位置失败");
        return false;
    }
    const HRESULT hr = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
    ILFree(pidl);
    if (FAILED(hr)) {
        status_.Error("打开所在位置失败");
        return false;
    }

    status_.Info("已打开所在位置");
    return true;
}

bool AppWindow::ShowSelectedItemShellMenu() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("请先选择条目");
        return false;
    }
    if (item->target_path.empty()) {
        status_.Warn("目标路径为空");
        return false;
    }

    const std::wstring path_w = launcher::util::Utf8ToWide(item->target_path);

    // 非文件系统目标（URL 等）退回属性页。
    PIDLIST_ABSOLUTE pidl_full = nullptr;
    const HRESULT parse_hr = SHParseDisplayName(path_w.c_str(), nullptr, &pidl_full, 0, nullptr);
    if (FAILED(parse_hr) || pidl_full == nullptr) {
        HINSTANCE instance = ShellExecuteW(m_hWnd, L"properties", path_w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(instance) <= 32) {
            status_.Error("打开资源管理器菜单失败");
            return false;
        }
        status_.Info("已打开属性页");
        return true;
    }

    // 真正的资源管理器右键菜单：SHBindToParent + IContextMenu。
    IShellFolder* parent_folder = nullptr;
    PCUITEMID_CHILD child_pidl = nullptr;
    IContextMenu* context_menu = nullptr;
    HMENU menu = nullptr;
    bool launched = false;

    auto release_all = [&]() {
        if (menu != nullptr) {
            DestroyMenu(menu);
            menu = nullptr;
        }
        if (context_menu != nullptr) {
            context_menu->Release();
            context_menu = nullptr;
        }
        if (parent_folder != nullptr) {
            parent_folder->Release();
            parent_folder = nullptr;
        }
        if (pidl_full != nullptr) {
            ILFree(pidl_full);
            pidl_full = nullptr;
        }
    };

    HRESULT hr = SHBindToParent(pidl_full, IID_IShellFolder, reinterpret_cast<void**>(&parent_folder), &child_pidl);
    if (FAILED(hr)) {
        release_all();
        status_.Error("打开资源管理器菜单失败");
        return false;
    }

    hr = parent_folder->GetUIObjectOf(m_hWnd, 1, &child_pidl, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&context_menu));
    if (FAILED(hr)) {
        release_all();
        status_.Error("打开资源管理器菜单失败");
        return false;
    }

    menu = CreatePopupMenu();
    constexpr UINT kScratchFirst = 0x7000;
    constexpr UINT kScratchLast = 0x7FFF;
    hr = context_menu->QueryContextMenu(menu, 0, kScratchFirst, kScratchLast, CMF_NORMAL);
    if (FAILED(hr)) {
        release_all();
        status_.Error("打开资源管理器菜单失败");
        return false;
    }

    POINT invoke_pt{0, 0};
    ::GetCursorPos(&invoke_pt);
    SetForegroundWindow(m_hWnd);
    const UINT command_id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                           invoke_pt.x, invoke_pt.y, 0, m_hWnd, nullptr);
    DestroyMenu(menu);
    menu = nullptr;

    if (command_id == 0) {
        release_all();
        return true; // 用户取消，不算失败
    }

    CMINVOKECOMMANDINFOEX invoke{};
    invoke.cbSize = sizeof(invoke);
    invoke.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
    invoke.hwnd = m_hWnd;
    invoke.lpVerb = MAKEINTRESOURCEA(command_id - kScratchFirst);
    invoke.lpVerbW = MAKEINTRESOURCEW(command_id - kScratchFirst);
    invoke.nShow = SW_SHOWNORMAL;
    invoke.ptInvoke = invoke_pt;
    hr = context_menu->InvokeCommand(reinterpret_cast<CMINVOKECOMMANDINFO*>(&invoke));
    release_all();

    if (FAILED(hr)) {
        status_.Error("执行资源管理器命令失败");
        return false;
    }

    status_.Info("已执行资源管理器命令");
    launched = true;
    return launched;
}

bool AppWindow::CopySelectedItemPath() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("请先选择条目");
        return false;
    }
    if (item->target_path.empty()) {
        status_.Warn("目标路径为空");
        return false;
    }

    const std::wstring text = launcher::util::Utf8ToWide(item->target_path);
    if (!OpenClipboard(m_hWnd)) {
        status_.Error("复制路径失败");
        return false;
    }

    EmptyClipboard();
    const std::size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (buffer == nullptr) {
        CloseClipboard();
        status_.Error("复制路径失败");
        return false;
    }
    void* ptr = GlobalLock(buffer);
    memcpy(ptr, text.c_str(), bytes);
    GlobalUnlock(buffer);
    SetClipboardData(CF_UNICODETEXT, buffer);
    CloseClipboard();

    status_.Info("路径已复制");
    return true;
}

bool AppWindow::MoveSelectedItemToGroup(const std::string& target_group_id) {
    if (selected_item_id_.empty()) {
        status_.Warn("请先选择条目");
        return false;
    }
    const std::string source_group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    if (source_group_id.empty()) {
        status_.Warn("源分组缺失");
        return false;
    }
    if (target_group_id == source_group_id) {
        status_.Warn("条目已在目标分组中");
        return false;
    }

    std::string error;
    if (!backend_.MoveItem(source_group_id, selected_item_id_, target_group_id, &error)) {
        status_.Error("移动条目失败：" + error);
        return false;
    }

    RenderGroups();
    RenderItems();
    status_.Info("条目已移动");
    return true;
}

void AppWindow::ExecuteSearchCommand(const std::string& item_id) {
    const std::string prefix = launcher::constants::kSearchCmdPrefix;
    if (item_id.rfind(prefix, 0) != 0) {
        return;
    }

    const int cmd_id = std::stoi(item_id.substr(prefix.size()));
    switch (cmd_id) {
    case launcher::constants::search_cmd::kCmd: {
        ShellExecuteW(nullptr, L"open", L"cmd.exe", nullptr, nullptr, SW_SHOWNORMAL);
        status_.Info("已打开命令提示符");
        break;
    }
    case launcher::constants::search_cmd::kSettings: {
        ShellExecuteW(nullptr, L"open", L"ms-settings:", nullptr, nullptr, SW_SHOWNORMAL);
        status_.Info("已打开系统设置");
        break;
    }
    case launcher::constants::search_cmd::kShutdown: {
        const int confirmed = MessageBoxW(m_hWnd, L"确定要关机吗？", L"关机", MB_ICONQUESTION | MB_YESNO);
        if (confirmed == IDYES) {
            system("shutdown /s /t 0");
            status_.Info("正在关机…");
        } else {
            status_.Info("已取消关机");
        }
        break;
    }
    case launcher::constants::search_cmd::kReboot: {
        const int confirmed = MessageBoxW(m_hWnd, L"确定要重启吗？", L"重启", MB_ICONQUESTION | MB_YESNO);
        if (confirmed == IDYES) {
            system("shutdown /r /t 0");
            status_.Info("正在重启…");
        } else {
            status_.Info("已取消重启");
        }
        break;
    }
    case launcher::constants::search_cmd::kLogoff: {
        const int confirmed = MessageBoxW(m_hWnd, L"确定要注销当前用户吗？", L"注销", MB_ICONQUESTION | MB_YESNO);
        if (confirmed == IDYES) {
            system("shutdown /l /t 0");
            status_.Info("正在注销…");
        } else {
            status_.Info("已取消注销");
        }
        break;
    }
    case launcher::constants::search_cmd::kScreenoff: {
        SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
        status_.Info("显示器已关闭");
        break;
    }
    case launcher::constants::search_cmd::kBaidu: {
        const std::wstring keyword = launcher::util::Utf8ToWide(search_controller_.GetBaiduKeyword());
        if (keyword.empty()) {
            ShellExecuteW(nullptr, L"open", L"https://www.baidu.com", nullptr, nullptr, SW_SHOWNORMAL);
        } else {
            std::wstring url = L"https://www.baidu.com/s?wd=" + keyword;
            ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        status_.Info("正在打开百度搜索");
        break;
    }
    default:
        break;
    }
}

void AppWindow::AddPresetSystemItem(UINT command_id) {
    namespace cmd = launcher::constants::command;

    core::ItemInput input;
    switch (command_id) {
    case cmd::kMainNewEmpty:
        input.name = "新项目";
        input.target_path = "";
        break;
    case cmd::kMainNewComputer:
        input.name = "计算机";
        input.target_path = "shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
        break;
    case cmd::kMainNewControlPanel:
        input.name = "控制面板";
        input.target_path = "shell:::{26EE0668-A00A-44D7-9371-BEB064C98683}";
        break;
    case cmd::kMainNewRecycleBin:
        input.name = "回收站";
        input.target_path = "shell:::{645FF040-5081-101B-9F08-00AA002F954E}";
        break;
    case cmd::kMainNewLogoff:
        input.name = "注销";
        input.target_path = "shutdown";
        input.arguments = "/l";
        break;
    case cmd::kMainNewShutdown:
        input.name = "关机";
        input.target_path = "shutdown";
        input.arguments = "/s /t 0";
        break;
    case cmd::kMainNewReboot:
        input.name = "重启";
        input.target_path = "shutdown";
        input.arguments = "/r /t 0";
        break;
    default:
        return;
    }

    const core::Group* group = FindActiveGroup();
    if (group == nullptr) {
        status_.Warn("请先选择分组");
        return;
    }
    std::string error;
    if (!backend_.UpsertItem(group->id, input, &error)) {
        status_.Error("创建失败：" + error);
        return;
    }
    RenderItems();
    status_.Info(std::string("已创建「") + input.name + "」");
}

void AppWindow::ToggleLayoutLock() {
    core::Settings next = backend_.CurrentSettings();
    next.locked = !next.locked;
    std::string error;
    if (!backend_.UpdateSettings(next, &error)) {
        status_.Error("保存设置失败：" + error);
        return;
    }
    layout_locked_ = next.locked;
    status_.Info(layout_locked_ ? "已锁定布局（拖动/缩放/重排已禁用）" : "已解除锁定");
}

void AppWindow::ToggleAutoHide() {
    core::Settings next = backend_.CurrentSettings();
    next.auto_hide = !next.auto_hide;
    std::string error;
    if (!backend_.UpdateSettings(next, &error)) {
        status_.Error("保存设置失败：" + error);
        return;
    }
    auto_hide_ = next.auto_hide;
    status_.Info(auto_hide_ ? "自动隐藏已开启（失焦隐藏，热键唤回）" : "自动隐藏已关闭");
}

void AppWindow::ConvertItemPathsMenu(bool to_relative) {
    const wchar_t* question = to_relative
        ? L"将扫描全部条目，把位于程序目录/同盘的路径转换为 %pr%/%cr% 占位符（便携模式）。\n转换前自动创建备份。继续吗？"
        : L"将扫描全部条目，把 %pr%/%cr% 占位符路径还原为绝对路径。\n转换前自动创建备份。继续吗？";
    if (MessageBoxW(m_hWnd, question, L"路径转换", MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return;
    }

    std::string error;
    const int converted = backend_.ConvertItemPaths(to_relative, &error);
    if (converted < 0) {
        status_.Error("路径转换失败：" + error);
        return;
    }
    if (converted == 0) {
        status_.Info("没有可转换的路径");
        return;
    }
    RenderItems();
    status_.Info("已转换 " + std::to_string(converted) + " 条路径"
        + (to_relative ? "（便携模式）" : "（绝对路径）"));
}
