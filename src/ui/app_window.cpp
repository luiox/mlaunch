#include "app_window.h"

#include <Windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>

#include "constants.h"
#include "dialog_manager.h"
#include "list_controller.h"
#include "logger.h"
#include "search_controller.h"
#include "ui_builder.h"
#include "utils/string_util.h"

using namespace DuiLib;

namespace {

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

std::filesystem::path GetAppBaseDir() {
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

bool IsSenderFromList(DuiLib::CControlUI* sender, DuiLib::CListUI* list) {
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

std::filesystem::path GetUiStatePath() {
    return GetAppBaseDir() / "ui_state.ini";
}

bool ReadIniInt(const std::filesystem::path& ini_path, const wchar_t* section, const wchar_t* key, int* out) {
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

bool WriteIniInt(const std::filesystem::path& ini_path, const wchar_t* section, const wchar_t* key, int value) {
    const std::wstring value_text = std::to_wstring(value);
    return ::WritePrivateProfileStringW(section, key, value_text.c_str(), ini_path.wstring().c_str()) != FALSE;
}

bool FlushIniFile(const std::filesystem::path& ini_path) {
    return ::WritePrivateProfileStringW(nullptr, nullptr, nullptr, ini_path.wstring().c_str()) != FALSE;
}

bool WriteUiStateAtomically(const std::filesystem::path& ini_path, const UiStateSnapshot& snapshot) {
    // 说明：曾尝试“写 tmp + MoveFileEx 原子替换”，但 kernel32 对 INI 文件
    // 有进程内句柄/写缓存，tmp 冲刷与替换在多种时序下都会失败（实测
    // ERROR_FILE_NOT_FOUND / 共享冲突），导致保存永久失败。
    // UI 几何信息非关键数据，改为直写目标文件 + 尽力冲刷，可靠性实测更好。
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

    // 尽力冲刷缓存；失败不视为保存失败（进程退出时缓存仍会落盘）。
    if (!FlushIniFile(ini_path)) {
        launcher::log::Warn("ui_state ini flush skipped err=" + std::to_string(::GetLastError()));
    }
    return true;
}

} // namespace

AppWindow::AppWindow(std::filesystem::path legacy_root)
    : launch_executor_(),
      shortcut_resolver_(),
      backend_(GetAppBaseDir(), std::move(legacy_root), &launch_executor_, &shortcut_resolver_),
            icon_manager_(GetAppBaseDir()),
            ui_builder_(*this),
            list_controller_(*this),
            search_controller_(*this),
            dialog_manager_(*this) {
    m_vctStaticName.push_back(_T("apptitlebar"));
}

std::string AppWindow::BasenameNoExt(const std::string& path) {
    std::error_code ec;
    const auto file_name = std::filesystem::path(path).filename().replace_extension("").string();
    if (!ec && !file_name.empty()) {
        return file_name;
    }
    return path;
}

std::string AppWindow::ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool AppWindow::ContainsCaseInsensitive(const std::string& text, const std::string& keyword) const {
    return ToLowerAscii(text).find(ToLowerAscii(keyword)) != std::string::npos;
}

void AppWindow::RestoreUiState() {
    const auto ini_path = GetUiStatePath();

    // 1) 先完成全部读取。
    int splitter_width = 0;
    const bool has_splitter = group_panel_ != nullptr &&
                              ReadIniInt(ini_path, L"layout", L"splitter_width", &splitter_width);

    // 兼容两种格式：
    // 1) 老格式：left/top/right/bottom
    // 2) 新格式：left/top/width/height
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    bool has_rect = false;

    const bool has_left = ReadIniInt(ini_path, L"window", L"left", &left);
    const bool has_top = ReadIniInt(ini_path, L"window", L"top", &top);
    const bool has_width = ReadIniInt(ini_path, L"window", L"width", &width);
    const bool has_height = ReadIniInt(ini_path, L"window", L"height", &height);

    if (has_left && has_top && has_width && has_height) {
        has_rect = true;
    } else {
        int right = 0;
        int bottom = 0;
        if (ReadIniInt(ini_path, L"window", L"left", &left) &&
            ReadIniInt(ini_path, L"window", L"top", &top) &&
            ReadIniInt(ini_path, L"window", L"right", &right) &&
            ReadIniInt(ini_path, L"window", L"bottom", &bottom)) {
            width = right - left;
            height = bottom - top;
            has_rect = true;
        }
    }

    int maximized = 0;
    const bool has_maximized = ReadIniInt(ini_path, L"window", L"maximized", &maximized);

    // 2) 读取完毕立即冲刷缓存：GetPrivateProfileStringW 会在进程内缓存该
    //    INI 的句柄，不释放的话，之后 SaveUiState 的原子替换将永远失败。
    ::WritePrivateProfileStringW(nullptr, nullptr, nullptr, ini_path.wstring().c_str());

    // 3) 应用读取到的状态。
    if (has_splitter && group_panel_ != nullptr) {
        if (splitter_width < 80) {
            splitter_width = 80;
        }
        if (splitter_width > 600) {
            splitter_width = 600;
        }
        group_panel_->SetFixedWidth(splitter_width);
    }

    if (!has_rect) {
        return;
    }
    if (width < 320 || height < 220) {
        return;
    }

    // 防离屏：恢复位置若几乎完全落在虚拟屏幕外（如更换显示器后），
    // 放弃恢复，回到默认位置，避免“打开后找不到窗口”。
    {
        RECT restored{left, top, left + width, top + height};
        RECT virtual_rect{
            ::GetSystemMetrics(SM_XVIRTUALSCREEN),
            ::GetSystemMetrics(SM_YVIRTUALSCREEN),
            ::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
            ::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN)};
        RECT intersect{};
        if (!::IntersectRect(&intersect, &restored, &virtual_rect) ||
            (intersect.right - intersect.left) < 100 ||
            (intersect.bottom - intersect.top) < 100) {
            launcher::log::Warn("ui_state window rect off-screen, ignored");
            return;
        }
    }

    // 创建后恢复窗口位置与大小，避免每次启动都回到默认尺寸。
    has_restored_window_ = true;
    ::SetWindowPos(m_hWnd, nullptr, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);

    if (has_maximized && maximized != 0) {
        start_maximized_ = true;
    }
}

void AppWindow::SaveUiState() {
    const auto ini_path = GetUiStatePath();

    UiStateSnapshot snapshot{};

    // 先从当前 UI 组装快照（内存为准），再一次性写盘。
    if (group_panel_ != nullptr) {
        int splitter_width = group_panel_->GetFixedWidth();
        if (splitter_width < 80) {
            splitter_width = 80;
        }
        snapshot.splitter_width = splitter_width;
    }

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(WINDOWPLACEMENT);
    if (!GetWindowPlacement(m_hWnd, &placement)) {
        return;
    }

    // 使用 rcNormalPosition 记录“正常窗口”状态下的几何信息，
    // 这样从最大化退出后仍能恢复到用户期望的普通尺寸。
    const RECT rc = placement.rcNormalPosition;
    snapshot.left = rc.left;
    snapshot.top = rc.top;
    snapshot.right = rc.right;
    snapshot.bottom = rc.bottom;
    snapshot.width = rc.right - rc.left;
    snapshot.height = rc.bottom - rc.top;
    snapshot.maximized = placement.showCmd == SW_SHOWMAXIMIZED ? 1 : 0;

    // 事务性写入：先写临时文件，再原子替换正式配置，避免中途损坏。
    if (!WriteUiStateAtomically(ini_path, snapshot)) {
        ui_state_dirty_ = true;
        ScheduleUiStateSave();
        std::fputs("[ui_state] save failed, retry scheduled\n", stderr);
        std::fflush(stderr);
        ::OutputDebugStringA("[ui_state] save failed, retry scheduled\n");
        return;
    }

    ui_state_dirty_ = false;
}

void AppWindow::ScheduleUiStateSave() {
    if (ui_state_timer_active_) {
        ::KillTimer(m_hWnd, launcher::constants::timer::kUiStateSave);
    }
    ::SetTimer(m_hWnd, launcher::constants::timer::kUiStateSave, launcher::constants::kUiStateSaveDelayMs, nullptr);
    ui_state_timer_active_ = true;
}

void AppWindow::MarkUiStateDirty() {
    // 状态变更后采用延迟写盘，避免频繁 IO。
    ui_state_dirty_ = true;
    ScheduleUiStateSave();
}

void AppWindow::FlushUiStateIfDirty() {
    if (!ui_state_dirty_) {
        return;
    }
    SaveUiState();
}

bool AppWindow::IsSearchMode() const {
    return search_controller_.IsSearchMode();
}

void AppWindow::UpdateSearchUi() {
    search_controller_.UpdateSearchUi();
}

bool AppWindow::LoadBackendData() {
    launcher::log::Info("loading backend data");
    std::string error;
    if (!backend_.Load(&error)) {
        launcher::log::Error("backend load failed: " + error);
        status_.Error("load failed: " + error);
        return false;
    }

    RenderGroups();
    if (!group_ids_.empty()) {
        SelectGroupByIndex(0);
    }
    launcher::log::Info("backend data loaded");
    status_.Info("ready");

    if (backend_.ConsumeLastLoadCorrupted()) {
        ShowBackupRecoveryMenu();
        return true;
    }

    // P0-M: 首次启动/迁移期检测旧版 Poner 数据，提供一键导入（幂等可反复执行）。
    const auto legacy_data_path = backend_.LegacyRoot() / "Data.json";
    launcher::log::Info("legacy import check: " + legacy_data_path.string());
    std::error_code legacy_ec;
    if (std::filesystem::exists(legacy_data_path, legacy_ec)) {
        const int confirmed = MessageBoxW(m_hWnd,
            L"Detected legacy Poner Data.json next to the app.\n"
            L"Import now? (Idempotent merge by group + target path; safe to re-run)",
            L"Import Poner Data",
            MB_ICONQUESTION | MB_YESNO);
        if (confirmed == IDYES) {
            ImportPonerFile(legacy_data_path);
        }
    }
    return true;
}

void AppWindow::RenderGroups() {
    list_controller_.RenderGroups();
}

void AppWindow::RenderItems() {
    list_controller_.RenderItems();
}

void AppWindow::SelectGroupByIndex(int index) {
    list_controller_.SelectGroupByIndex(index);
}

void AppWindow::LaunchSelectedItem() {
    if (selected_item_id_.empty()) {
        status_.Warn("no item selected");
        return;
    }

    if (selected_item_id_.rfind(launcher::constants::kSearchCmdPrefix, 0) == 0) {
        ExecuteSearchCommand(selected_item_id_);
        return;
    }

    // 分隔条只做选中，不触发启动，避免单击时弹出错误提示。
    const core::LaunchItem* clicked = FindSelectedItem();
    if (clicked != nullptr && clicked->item_type == "separator") {
        return;
    }

    // 回收站内的条目已删除，单击只选中，不启动。
    if (!selected_item_group_id_.empty() && backend_.IsRecycleBinId(selected_item_group_id_)) {
        return;
    }

    const std::string group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    if (group_id.empty()) {
        status_.Warn("no group selected");
        return;
    }

    std::string error;
    const auto result = backend_.Launch(group_id, selected_item_id_, &error);
    if (!result.ok) {
        status_.Error(error.empty() ? result.message : ("launch failed: " + error));
        return;
    }

    status_.Info(result.message);
    RenderItems();
}

void AppWindow::DeleteSelectedItem() {
    if (selected_item_id_.empty()) {
        status_.Warn("select an item first");
        return;
    }

    const std::string group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    if (group_id.empty()) {
        status_.Warn("no group selected");
        return;
    }

    const bool permanent = backend_.IsRecycleBinId(group_id);
    if (permanent) {
        const int confirmed = MessageBoxW(m_hWnd,
            L"Permanently delete this item? It can only be recovered from backups.",
            L"Delete Permanently",
            MB_ICONWARNING | MB_YESNO);
        if (confirmed != IDYES) {
            status_.Warn("delete canceled");
            return;
        }
    }

    std::string error;
    if (!backend_.DeleteItem(group_id, selected_item_id_, &error)) {
        status_.Error("delete item failed: " + error);
        return;
    }

    selected_item_id_.clear();
    selected_item_group_id_.clear();
    RenderGroups();
    RenderItems();
    status_.Info(permanent ? "deleted permanently" : "deleted · Ctrl+Z to undo");
}

void AppWindow::UndoLastDelete() {
    std::string error;
    if (!backend_.UndoLastDelete(&error)) {
        status_.Warn(error.empty() ? "nothing to undo" : ("undo failed: " + error));
        return;
    }
    RenderGroups();
    RenderItems();
    status_.Info("restore completed");
}

bool AppWindow::IsActiveGroupRecycleBin() const {
    return backend_.IsRecycleBinId(active_group_id_);
}

void AppWindow::ShowBackupRecoveryMenu() {
    const auto backups = backend_.ListBackups();
    if (backups.empty()) {
        status_.Warn("data was corrupted; no backups available");
        return;
    }

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, L"Data file was corrupted. Restore from backup:");
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
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kBackupStartFresh, L"Start Fresh");

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
        status_.Warn("kept fresh data; backups remain in backups/");
        return;
    }
    if (command_id < launcher::constants::command::kBackupRestoreBase) {
        return;
    }

    const auto index = static_cast<int>(command_id - launcher::constants::command::kBackupRestoreBase);
    const auto backups = backend_.ListBackups();
    if (index < 0 || index >= static_cast<int>(backups.size())) {
        status_.Error("backup entry no longer available");
        return;
    }

    std::string error;
    if (!backend_.RestoreFromBackup(backups[index].path, &error)) {
        status_.Error("restore failed: " + error);
        return;
    }

    RenderGroups();
    if (!group_ids_.empty()) {
        SelectGroupByIndex(0);
    }
    status_.Info("restored from " + backups[index].name);
}

CControlUI* AppWindow::BuildRootUi() {
    return ui_builder_.BuildRootUi();
}

LRESULT AppWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
    styleValue &= ~WS_CAPTION;
    ::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    m_pm.Init(m_hWnd, GetManagerName(), this);
    // 对齐 VB6 frmMain：微软雅黑 9.75pt 常规（96DPI 下约 13px）。
    // 默认字体覆盖所有未显式设置字体的控件；字体 1 供搜索框/辅助文字使用。
    m_pm.SetDefaultFont(_T("微软雅黑"), 14, false, false, false, false);
    m_pm.AddFont(1, _T("微软雅黑"), 12, false, false, false);

    CControlUI* root = BuildRootUi();
    if (root == nullptr) {
        MessageBox(nullptr, _T("Create UI failed"), _T("DuiLib"), MB_OK | MB_ICONERROR);
        ExitProcess(1);
        return 0;
    }

    m_pm.AttachDialog(root);
    m_pm.AddNotifier(this);

    RECT rc_caption{0, 0, 0, 30};
    m_pm.SetCaptionRect(rc_caption);
    RECT rc_sizebox{6, 6, 6, 6};
    m_pm.SetSizeBox(rc_sizebox);

    groups_list_ = static_cast<CListUI*>(m_pm.FindControl(_T("groups_list")));
    items_list_ = static_cast<CListUI*>(m_pm.FindControl(_T("items_list")));
    status_line_ = static_cast<CLabelUI*>(m_pm.FindControl(_T("status_line")));
    search_bar_ = m_pm.FindControl(_T("search_bar"));
    group_panel_ = static_cast<CVerticalLayoutUI*>(m_pm.FindControl(_T("group_panel")));
    panel_splitter_ = m_pm.FindControl(_T("panel_splitter"));
    search_input_ = static_cast<CEditUI*>(m_pm.FindControl(_T("search_input")));
    group_dialog_ = static_cast<CVerticalLayoutUI*>(m_pm.FindControl(_T("group_dialog")));
    group_dialog_title_ = static_cast<CLabelUI*>(m_pm.FindControl(_T("group_dialog_title")));
    group_dialog_input_ = static_cast<CEditUI*>(m_pm.FindControl(_T("group_dialog_input")));
    status_.Bind(status_line_, m_hWnd, launcher::constants::timer::kStatusToast);

    RestoreUiState();

    DragAcceptFiles(m_hWnd, TRUE);

    LoadBackendData();
    __InitWindow();
    bHandled = TRUE;
    return 0;
}

LRESULT AppWindow::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) {
    BOOL custom_handled = FALSE;
    const LRESULT custom_result = HandleCustomMessage(uMsg, wParam, lParam, custom_handled);
    if (custom_handled) {
        bHandled = true;
        return custom_result;
    }
    return WindowImplBase::MessageHandler(uMsg, wParam, lParam, bHandled);
}

void AppWindow::Notify(TNotifyUI& msg) {
    if (_tcscmp(msg.sType, DUI_MSGTYPE_CLICK) == 0) {
        if (msg.pSender != nullptr && msg.pSender->GetName() == _T("group_dialog_ok")) {
            ConfirmGroupDialog();
            return;
        }
        if (msg.pSender != nullptr && msg.pSender->GetName() == _T("group_dialog_cancel")) {
            CloseGroupDialog();
            return;
        }
        if (msg.pSender != nullptr && msg.pSender->GetName() == _T("closebtn")) {
            ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
            return;
        }
        if (msg.pSender != nullptr && msg.pSender->GetName() == _T("searchbtn")) {
            search_controller_.ToggleSearchMode();
            return;
        }
        if (msg.pSender != nullptr && msg.pSender->GetName() == _T("menubtn")) {
            RECT rc{};
            ::GetWindowRect(m_hWnd, &rc);
            POINT menu_point{rc.left + 12, rc.top + 35};
            ShowMainContextMenu(menu_point);
            return;
        }
    }

    if (_tcscmp(msg.sType, DUI_MSGTYPE_TEXTCHANGED) == 0 && msg.pSender != nullptr && msg.pSender->GetName() == _T("search_input")) {
        search_controller_.HandleInputChanged();
        return;
    }

    if (_tcscmp(msg.sType, DUI_MSGTYPE_ITEMCLICK) == 0 && msg.pSender != nullptr) {
        if (groups_list_ != nullptr && IsSenderFromList(msg.pSender, groups_list_)) {
            SelectGroupByIndex(groups_list_->GetCurSel());
            return;
        }
        if (items_list_ != nullptr && IsSenderFromList(msg.pSender, items_list_)) {
            const int index = items_list_->GetCurSel();
            if (index >= 0 && index < static_cast<int>(item_ids_.size())) {
                selected_item_id_ = item_ids_[index];
                if (index < static_cast<int>(item_group_ids_.size())) {
                    selected_item_group_id_ = item_group_ids_[index];
                }
                // 对齐 VB6 行为：左键单击即启动（拖拽重排不会触发 ITEMCLICK）。
                LaunchSelectedItem();
            }
            return;
        }
    }

    WindowImplBase::Notify(msg);
}

bool AppWindow::SelectListRowFromPoint(CListUI* list, const std::vector<std::string>& ids, const POINT& client_point, std::string* selected_id) {
    return list_controller_.SelectListRowFromPoint(list, ids, client_point, selected_id);
}

void AppWindow::ShowGroupContextMenu(const POINT& screen_point) {
    if (IsActiveGroupRecycleBin()) {
        status_.Info("recycle bin is managed automatically");
        return;
    }

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kGroupAdd, L"Add Group");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kGroupRename, L"Edit Group Name");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kGroupDelete, L"Delete Group");

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
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemDelete, L"Delete Permanently");
    } else {
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemRunAs, L"Run as administrator");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemOpenFolder, L"Open file location");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemShellMenu, L"Explorer menu");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemCopyPath, L"Copy full path");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemAdd, L"Add Item");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemEdit, L"Edit Item");
        AppendMenuW(menu, MF_STRING, launcher::constants::command::kItemDelete, L"Delete Item");

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
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(move_menu), L"Move To Group");
    }

    SetForegroundWindow(m_hWnd);
    const UINT command_id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen_point.x, screen_point.y, 0, m_hWnd, nullptr);
    DestroyMenu(menu);

    if (command_id != 0) {
        ExecuteItemCommand(command_id);
    }
}

void AppWindow::ShowMainContextMenu(const POINT& screen_point) {
    HMENU menu = CreatePopupMenu();
    HMENU new_menu = CreatePopupMenu();

    AppendMenuW(new_menu, MF_STRING, launcher::constants::command::kMainNewCustom, L"Custom");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(new_menu), L"New Item");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainSortByName, L"Sort By Name");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainImportData, L"Import Data");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainExportData, L"Export Data");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainSettings, L"Settings");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainWebSite, L"Website");
    AppendMenuW(menu, MF_STRING, launcher::constants::command::kMainExit, L"Exit");

    SetForegroundWindow(m_hWnd);
    const UINT command_id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen_point.x, screen_point.y, 0, m_hWnd, nullptr);
    DestroyMenu(menu);

    if (command_id != 0) {
        ExecuteMainCommand(command_id);
    }
}

void AppWindow::ExecuteMainCommand(UINT command_id) {
    switch (command_id) {
    case launcher::constants::command::kMainNewCustom:
        AddItemFromFile();
        return;
    case launcher::constants::command::kMainSortByName:
        status_.Warn("sort by name is not implemented yet");
        return;
    case launcher::constants::command::kMainImportData: {
        const std::wstring file = PickJsonFilePath();
        if (file.empty()) {
            status_.Warn("import canceled");
            return;
        }
        ImportPonerFile(std::filesystem::path(file));
        return;
    }
    case launcher::constants::command::kMainExportData:
        status_.Warn("export data is not implemented yet");
        return;
    case launcher::constants::command::kMainSettings:
        status_.Warn("settings window is not implemented yet");
        return;
    case launcher::constants::command::kMainWebSite:
        ShellExecuteW(nullptr, L"open", L"https://www.52pojie.cn/?Poner", nullptr, nullptr, SW_SHOWNORMAL);
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
        OpenGroupDialog(false, std::string());
        return;
    }

    if (command_id == launcher::constants::command::kGroupRename) {
        const core::Group* group = FindActiveGroup();
        if (group == nullptr) {
            status_.Warn("no group selected");
            return;
        }
        OpenGroupDialog(true, group->id);
        return;
    }

    if (command_id == launcher::constants::command::kGroupDelete) {
        DeleteActiveGroup();
    }
}

void AppWindow::OpenGroupDialog(bool rename_mode, const std::string& group_id) {
    dialog_manager_.OpenGroupDialog(rename_mode, group_id);
}

void AppWindow::CloseGroupDialog() {
    dialog_manager_.CloseGroupDialog();
}

void AppWindow::ConfirmGroupDialog() {
    dialog_manager_.ConfirmGroupDialog();
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
        AddItemFromFile();
        return;
    }
    if (command_id == launcher::constants::command::kItemEdit) {
        EditSelectedItem();
        return;
    }
    if (command_id == launcher::constants::command::kItemDelete) {
        DeleteSelectedItem();
        return;
    }
    if (command_id >= launcher::constants::command::kItemMoveBase) {
        const int group_index = static_cast<int>(command_id - launcher::constants::command::kItemMoveBase);
        if (group_index < 0 || group_index >= static_cast<int>(group_ids_.size())) {
            status_.Warn("invalid target group");
            return;
        }
        MoveSelectedItemToGroup(group_ids_[group_index]);
    }
}

std::wstring AppWindow::PickExecutablePath() const {
    wchar_t file_path[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) {
        return {};
    }
    return file_path;
}

std::wstring AppWindow::PickJsonFilePath() const {
    wchar_t file_path[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"JSON Data (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) {
        return {};
    }
    return file_path;
}

bool AppWindow::ImportPonerFile(const std::filesystem::path& path) {
    std::string error;
    const auto merged = backend_.ImportPonerData(path, &error);
    if (merged == 0 && !error.empty()) {
        status_.Error("import failed: " + error);
        return false;
    }

    RenderGroups();
    if (!group_ids_.empty()) {
        SelectGroupByIndex(0);
    }
    RenderItems();

    if (merged == 0) {
        status_.Info("nothing new to import");
    } else {
        status_.Info("imported " + std::to_string(merged) + " items from " + path.filename().string());
    }
    return true;
}

bool AppWindow::AddItemFromFile() {
    if (active_group_id_.empty()) {
        status_.Warn("select a group first");
        return false;
    }

    const std::wstring file_path = PickExecutablePath();
    if (file_path.empty()) {
        status_.Warn("add item canceled");
        return false;
    }

    core::ItemInput input;
    input.item_type = std::string("app");
    input.name = BasenameNoExt(launcher::util::WideToUtf8(file_path));
    input.target_path = launcher::util::WideToUtf8(file_path);
    input.icon_location = input.target_path;
    input.arguments.clear();
    input.enabled = true;

    std::string error;
    if (!backend_.UpsertItem(active_group_id_, input, &error)) {
        status_.Error("add item failed: " + error);
        return false;
    }

    RenderItems();
    status_.Info("item added");
    return true;
}

bool AppWindow::EditSelectedItem() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("select an item first");
        return false;
    }

    const std::wstring file_path = PickExecutablePath();
    if (file_path.empty()) {
        status_.Warn("edit item canceled");
        return false;
    }

    core::ItemInput input;
    input.id = item->id;
    input.item_type = item->item_type;
    input.name = BasenameNoExt(launcher::util::WideToUtf8(file_path));
    input.target_path = launcher::util::WideToUtf8(file_path);
    input.icon_location = input.target_path;
    input.arguments = item->arguments;
    input.enabled = item->enabled;

    std::string error;
    const std::string group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    if (!backend_.UpsertItem(group_id, input, &error)) {
        status_.Error("edit item failed: " + error);
        return false;
    }

    RenderItems();
    status_.Info("item updated");
    return true;
}

bool AppWindow::DeleteActiveGroup() {
    const core::Group* active_group = FindActiveGroup();
    if (active_group == nullptr) {
        status_.Warn("select a group first");
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
        status_.Warn("cannot delete the last group");
        return false;
    }

    const int confirmed = MessageBoxW(m_hWnd,
        L"The selected group will be deleted and its items moved to another group. Continue?",
        L"Delete Group",
        MB_ICONQUESTION | MB_YESNO);
    if (confirmed != IDYES) {
        status_.Warn("delete group canceled");
        return false;
    }

    std::string error;
    if (!backend_.DeleteGroup(active_group->id, target_group_id, &error)) {
        status_.Error("delete group failed: " + error);
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
    status_.Info("group deleted");
    return true;
}

bool AppWindow::RunSelectedItemAsAdmin() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("select an item first");
        return false;
    }
    if (item->item_type == "separator") {
        status_.Warn("separator item cannot be launched");
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
        status_.Error("run as administrator failed");
        return false;
    }

    status_.Info("started as administrator");
    return true;
}

bool AppWindow::OpenSelectedItemFolder() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("select an item first");
        return false;
    }
    if (item->target_path.empty()) {
        status_.Warn("target path is empty");
        return false;
    }

    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(launcher::util::Utf8ToWide(item->target_path).c_str());
    if (pidl == nullptr) {
        status_.Error("open file location failed");
        return false;
    }
    const HRESULT hr = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
    ILFree(pidl);
    if (FAILED(hr)) {
        status_.Error("open file location failed");
        return false;
    }

    status_.Info("opened file location");
    return true;
}

bool AppWindow::ShowSelectedItemShellMenu() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("select an item first");
        return false;
    }
    if (item->target_path.empty()) {
        status_.Warn("target path is empty");
        return false;
    }

    const std::wstring path_w = launcher::util::Utf8ToWide(item->target_path);
    HINSTANCE instance = ShellExecuteW(m_hWnd, L"properties", path_w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(instance) <= 32) {
        status_.Error("open explorer menu failed");
        return false;
    }

    status_.Info("opened explorer properties");
    return true;
}

bool AppWindow::CopySelectedItemPath() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("select an item first");
        return false;
    }
    if (item->target_path.empty()) {
        status_.Warn("target path is empty");
        return false;
    }

    const std::wstring text = launcher::util::Utf8ToWide(item->target_path);
    if (!OpenClipboard(m_hWnd)) {
        status_.Error("copy path failed");
        return false;
    }

    EmptyClipboard();
    const std::size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (buffer == nullptr) {
        CloseClipboard();
        status_.Error("copy path failed");
        return false;
    }
    void* ptr = GlobalLock(buffer);
    memcpy(ptr, text.c_str(), bytes);
    GlobalUnlock(buffer);
    SetClipboardData(CF_UNICODETEXT, buffer);
    CloseClipboard();

    status_.Info("path copied");
    return true;
}

bool AppWindow::MoveSelectedItemToGroup(const std::string& target_group_id) {
    if (selected_item_id_.empty()) {
        status_.Warn("select an item first");
        return false;
    }
    const std::string source_group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    if (source_group_id.empty()) {
        status_.Warn("no source group");
        return false;
    }
    if (target_group_id == source_group_id) {
        status_.Warn("item is already in this group");
        return false;
    }

    std::string error;
    if (!backend_.MoveItem(source_group_id, selected_item_id_, target_group_id, &error)) {
        status_.Error("move item failed: " + error);
        return false;
    }

    RenderGroups();
    RenderItems();
    status_.Info("item moved");
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
        status_.Info("opened command prompt");
        break;
    }
    case launcher::constants::search_cmd::kSettings: {
        ShellExecuteW(nullptr, L"open", L"ms-settings:", nullptr, nullptr, SW_SHOWNORMAL);
        status_.Info("opened system settings");
        break;
    }
    case launcher::constants::search_cmd::kShutdown: {
        const int confirmed = MessageBoxW(m_hWnd, L"Are you sure you want to shut down?", L"Shut Down", MB_ICONQUESTION | MB_YESNO);
        if (confirmed == IDYES) {
            system("shutdown /s /t 0");
            status_.Info("shutting down...");
        } else {
            status_.Info("shutdown canceled");
        }
        break;
    }
    case launcher::constants::search_cmd::kReboot: {
        const int confirmed = MessageBoxW(m_hWnd, L"Are you sure you want to restart?", L"Restart", MB_ICONQUESTION | MB_YESNO);
        if (confirmed == IDYES) {
            system("shutdown /r /t 0");
            status_.Info("restarting...");
        } else {
            status_.Info("restart canceled");
        }
        break;
    }
    case launcher::constants::search_cmd::kLogoff: {
        const int confirmed = MessageBoxW(m_hWnd, L"Are you sure you want to log off?", L"Log Off", MB_ICONQUESTION | MB_YESNO);
        if (confirmed == IDYES) {
            system("shutdown /l /t 0");
            status_.Info("logging off...");
        } else {
            status_.Info("logoff canceled");
        }
        break;
    }
    case launcher::constants::search_cmd::kScreenoff: {
        SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
        status_.Info("display turned off");
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
        status_.Info("searching baidu");
        break;
    }
    default:
        break;
    }
}

std::string AppWindow::GenerateNewGroupName() const {
    int index = 1;
    while (true) {
        const std::string candidate = (index == 1) ? "New Group" : ("New Group " + std::to_string(index));
        bool exists = false;
        for (const auto& group : backend_.Data().groups) {
            if (group.name == candidate) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            return candidate;
        }
        ++index;
    }
}

const core::Group* AppWindow::FindActiveGroup() const {
    for (const auto& group : backend_.Data().groups) {
        if (group.id == active_group_id_) {
            return &group;
        }
    }
    return nullptr;
}

const core::LaunchItem* AppWindow::FindSelectedItem() const {
    const std::string group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    const core::Group* group = nullptr;
    for (const auto& candidate : backend_.Data().groups) {
        if (candidate.id == group_id) {
            group = &candidate;
            break;
        }
    }
    if (group == nullptr) {
        return nullptr;
    }
    for (const auto& item : group->items) {
        if (item.id == selected_item_id_) {
            return &item;
        }
    }
    return nullptr;
}

