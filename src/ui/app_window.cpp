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

#include "app_window_internal.h"
#include "constants.h"
#include "dialog_manager.h"
#include "list_controller.h"
#include "logger.h"
#include "search_controller.h"
#include "ui_builder.h"
#include "utils/string_util.h"

#include <dwmapi.h>

using namespace DuiLib;

AppWindow::AppWindow(std::filesystem::path legacy_root)
    : launch_executor_(),
      shortcut_resolver_(),
      backend_(appwin::GetAppBaseDir(), std::move(legacy_root), &launch_executor_, &shortcut_resolver_),
      icon_manager_(appwin::GetAppBaseDir()),
      ui_builder_(*this),
      list_controller_(*this),
      search_controller_(*this),
      dialog_manager_(*this) {
    backend_.SetAppDir(GetExeDir());
    m_vctStaticName.push_back(_T("apptitlebar"));
}

std::filesystem::path AppWindow::GetExeDir() const {
    wchar_t buffer[MAX_PATH]{};
    ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
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
        status_.Error("数据加载失败：" + error);
        return false;
    }

    RenderGroups();
    if (!group_ids_.empty()) {
        SelectGroupByIndex(0);
    }
    launcher::log::Info("backend data loaded");
    status_.Info("就绪");

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
            L"检测到程序目录下存在旧版 Poner Data.json。\n"
            L"是否立即导入？（按 分组+目标路径 幂等合并，可安全重复执行）",
            L"导入 Poner 数据",
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
        status_.Warn("未选中条目");
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
        status_.Warn("未选中分组");
        return;
    }

    std::string error;
    const auto result = backend_.Launch(group_id, selected_item_id_, &error);
    if (!result.ok) {
        status_.Error(error.empty() ? result.message : ("启动失败：" + error));
        return;
    }

    status_.Info(result.message);
    RenderItems();

    // 对齐 VB6 原版行为：执行成功后主窗口最小化（进任务栏，可点击还原），而非隐藏。
    if (backend_.CurrentSettings().execute_hide) {
        ::ShowWindow(m_hWnd, SW_MINIMIZE);
    }
}

void AppWindow::DeleteSelectedItem() {
    if (selected_item_id_.empty()) {
        status_.Warn("请先选择条目");
        return;
    }

    const std::string group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    if (group_id.empty()) {
        status_.Warn("未选中分组");
        return;
    }

    const bool permanent = backend_.IsRecycleBinId(group_id);
    if (permanent) {
        const int confirmed = MessageBoxW(m_hWnd,
            L"彻底删除该条目？此后仅可通过备份找回。",
            L"彻底删除",
            MB_ICONWARNING | MB_YESNO);
        if (confirmed != IDYES) {
            status_.Warn("已取消删除");
            return;
        }
    }

    std::string error;
    if (!backend_.DeleteItem(group_id, selected_item_id_, &error)) {
        status_.Error("删除条目失败：" + error);
        return;
    }

    selected_item_id_.clear();
    selected_item_group_id_.clear();
    RenderGroups();
    RenderItems();
    status_.Info(permanent ? "已彻底删除" : "已删除 · Ctrl+Z 撤销");
}

void AppWindow::UndoLastDelete() {
    std::string error;
    if (!backend_.UndoLastDelete(&error)) {
        status_.Warn(error.empty() ? "没有可撤销的删除" : ("撤销失败：" + error));
        return;
    }
    RenderGroups();
    RenderItems();
    status_.Info("已恢复原位置");
}

bool AppWindow::IsActiveGroupRecycleBin() const {
    return backend_.IsRecycleBinId(active_group_id_);
}

CControlUI* AppWindow::BuildRootUi() {
    return ui_builder_.BuildRootUi();
}

LRESULT AppWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
    styleValue &= ~WS_CAPTION;
    ::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    m_pm.Init(m_hWnd, GetManagerName(), this);
    // 原生 EDIT 子窗口内的键盘消息（如搜索框 ESC）在这层拦截（见 TranslateAccelerator）。
    m_pm.AddTranslateAccelerator(this);
    // 对齐 VB6 frmMain：微软雅黑 9.75pt 常规（96DPI 下约 13px）。
    // 默认字体覆盖所有未显式设置字体的控件；字体 1 供搜索框/辅助文字使用。
    m_pm.SetDefaultFont(_T("微软雅黑"), 14, false, false, false, false);
    m_pm.AddFont(1, _T("微软雅黑"), 12, false, false, false);

    CControlUI* root = BuildRootUi();
    if (root == nullptr) {
        MessageBox(nullptr, _T("创建界面失败"), _T("DuiLib"), MB_OK | MB_ICONERROR);
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
    search_button_ = static_cast<appui::IconButtonUI*>(m_pm.FindControl(_T("searchbtn")));
    group_panel_ = static_cast<CVerticalLayoutUI*>(m_pm.FindControl(_T("group_panel")));
    panel_splitter_ = m_pm.FindControl(_T("panel_splitter"));
    search_input_ = static_cast<CEditUI*>(m_pm.FindControl(_T("search_input")));
    group_dialog_ = static_cast<CVerticalLayoutUI*>(m_pm.FindControl(_T("group_dialog")));
    group_dialog_title_ = static_cast<CLabelUI*>(m_pm.FindControl(_T("group_dialog_title")));
    group_dialog_input_ = static_cast<CEditUI*>(m_pm.FindControl(_T("group_dialog_input")));
    status_.Bind(status_line_, m_hWnd, launcher::constants::timer::kStatusToast);

    DragAcceptFiles(m_hWnd, TRUE);

    // 先加载数据与设置，RestoreUiState 才能用设置里的默认分组栏宽度。
    LoadBackendData();
    RestoreUiState();
    RegisterConfiguredHotkey();
    // 锁定/自动隐藏等行为开关随设置生效。
    layout_locked_ = backend_.CurrentSettings().locked;
    auto_hide_ = backend_.CurrentSettings().auto_hide;
    // 开机自启注册表每次启动按设置对齐（exe 挪位后修正路径/清残留）。
    ApplyAutorunRegistry(backend_.CurrentSettings().autorun);

    // 复刻 VB6 frmShadow：关掉 DWM 软阴影，改用主窗后方的硬边偏移剪影。
    {
        const DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
        ::DwmSetWindowAttribute(m_hWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    }
    shadow_window_.Attach(m_hWnd);
    shadow_window_.Sync();

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

LRESULT AppWindow::TranslateAccelerator(MSG* pMsg) {
    // 搜索框的原生 EDIT 持有焦点时 WM_KEYDOWN 只到 EDIT 子窗口（Win32 机制），
    // 在 fork 消息循环派发前拦截 ESC 退出搜索模式与上/下方向键移动搜索结果选择；
    // 键位→业务动作属宿主职责，不下沉进 DuiLib。返回约定见头文件：S_OK 吞掉、S_FALSE 放行。
    if (search_mode_ && pMsg != nullptr && pMsg->message == WM_KEYDOWN
        && pMsg->hwnd != nullptr && pMsg->hwnd != m_hWnd
        && ::GetAncestor(pMsg->hwnd, GA_ROOT) == m_hWnd) {
        if (pMsg->wParam == VK_ESCAPE) {
            search_controller_.ToggleSearchMode();
            return S_OK;
        }
        if (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN) {
            search_controller_.MoveSearchSelection(pMsg->wParam == VK_UP ? -1 : 1);
            return S_OK;
        }
    }
    return S_FALSE;
}

void AppWindow::SelectItemByIndex(int index) {
    if (items_list_ == nullptr || index < 0 || index >= items_list_->GetCount()) {
        return;
    }
    if (index >= static_cast<int>(item_ids_.size()) || item_ids_[index].empty()) {
        return; // 搜索空输入的提示占位行不可选。
    }
    selected_item_id_ = item_ids_[index];
    selected_item_group_id_ = index < static_cast<int>(item_group_ids_.size())
                                  ? item_group_ids_[index]
                                  : std::string();
    items_list_->SelectItem(index, false);
    items_list_->EnsureVisible(index);
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
            // 对齐原版：菜单贴在 ☰ 按钮正下方（按钮右缘对齐），不弹到窗口左侧。
            POINT menu_point{0, 0};
            if (msg.pSender != nullptr) {
                const RECT btn_rc = msg.pSender->GetPos();
                POINT client_pt{btn_rc.right, btn_rc.bottom};
                ::ClientToScreen(m_hWnd, &client_pt);
                menu_point = client_pt;
            }
            ShowMainContextMenu(menu_point, true);
            return;
        }
    }

    if (_tcscmp(msg.sType, DUI_MSGTYPE_TEXTCHANGED) == 0 && msg.pSender != nullptr && msg.pSender->GetName() == _T("search_input")) {
        search_controller_.HandleInputChanged();
        return;
    }

    // 搜索框持有焦点时，回车由原生 EDIT 转成通知送到这里（fork 基线行为）。
    if (msg.pSender != nullptr && msg.pSender->GetName() == _T("search_input")) {
        if (_tcscmp(msg.sType, DUI_MSGTYPE_RETURN) == 0) {
            if (search_mode_) {
                LaunchSelectedItem();
            }
            return;
        }
    }

    if (_tcscmp(msg.sType, DUI_MSGTYPE_ITEMCLICK) == 0 && msg.pSender != nullptr) {
        if (groups_list_ != nullptr && appwin::IsSenderFromList(msg.pSender, groups_list_)) {
            SelectGroupByIndex(groups_list_->GetCurSel());
            return;
        }
        if (items_list_ != nullptr && appwin::IsSenderFromList(msg.pSender, items_list_)) {
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

void AppWindow::OpenGroupDialog(bool rename_mode, const std::string& group_id) {
    dialog_manager_.OpenGroupDialog(rename_mode, group_id);
}

void AppWindow::CloseGroupDialog() {
    dialog_manager_.CloseGroupDialog();
}

void AppWindow::ConfirmGroupDialog() {
    dialog_manager_.ConfirmGroupDialog();
}

void AppWindow::OpenSettingsDialog() {
    dialog_manager_.OpenSettingsDialog();
}

void AppWindow::CloseSettingsDialog() {
    dialog_manager_.CloseSettingsDialog();
}

void AppWindow::OpenItemDialog(bool edit_mode) {
    if (edit_mode) {
        const std::string group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
        if (group_id.empty() || selected_item_id_.empty()) {
            status_.Warn("请先选择条目");
            return;
        }
        dialog_manager_.OpenItemDialog(true, group_id, selected_item_id_);
    } else {
        if (active_group_id_.empty()) {
            status_.Warn("请先选择分组");
            return;
        }
        if (backend_.IsRecycleBinId(active_group_id_)) {
            status_.Warn("回收站内不能添加项目");
            return;
        }
        dialog_manager_.OpenItemDialog(false, active_group_id_, std::string());
    }
}

void AppWindow::CloseItemDialog() {
    dialog_manager_.CloseItemDialog();
}

void AppWindow::ApplySettings() {
    RegisterConfiguredHotkey();
    ApplyAutorunRegistry(backend_.CurrentSettings().autorun);

    layout_locked_ = backend_.CurrentSettings().locked;
    auto_hide_ = backend_.CurrentSettings().auto_hide;

    if (group_panel_ != nullptr) {
        int panel_width = static_cast<int>(backend_.CurrentSettings().group_panel_width);
        panel_width = std::clamp(panel_width, 80, 600);
        group_panel_->SetFixedWidth(panel_width);
    }
    m_pm.NeedUpdate();
}

void AppWindow::ApplyAutorunRegistry(bool enabled) {
    // 开机自启 = HKCU Run 键注册表项；每次启动按设置同步（exe 挪位后自愈路径）。
    HKEY key = nullptr;
    if (::RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                          0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        launcher::log::Warn("autorun: open Run key failed err=" + std::to_string(::GetLastError()));
        return;
    }
    if (enabled) {
        wchar_t exe_path[MAX_PATH] = {};
        const DWORD path_len = ::GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        if (path_len == 0 || path_len == MAX_PATH) {
            launcher::log::Warn("autorun: GetModuleFileName failed err=" + std::to_string(::GetLastError()));
            ::RegCloseKey(key);
            return;
        }
        const std::wstring value = L"\"" + std::wstring(exe_path, path_len) + L"\"";
        const LSTATUS status = ::RegSetKeyValueW(key, nullptr, L"mlaunch", REG_SZ, value.c_str(),
                                                 static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        if (status != ERROR_SUCCESS) {
            launcher::log::Warn("autorun: RegSetKeyValue failed err=" + std::to_string(status));
        }
    } else {
        // 关闭/默认态都删除，顺带清掉历史遗留项。
        ::RegDeleteKeyValueW(key, nullptr, L"mlaunch");
    }
    ::RegCloseKey(key);
}

LRESULT AppWindow::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    const LRESULT hit = WindowImplBase::OnNcHitTest(uMsg, wParam, lParam, bHandled);
    if (!layout_locked_ || !bHandled) {
        return hit;
    }
    // 锁定布局：吞掉 caption 拖动与四边/四角缩放命中，其余（关闭按钮等客户区）不受影响。
    switch (hit) {
    case HTCAPTION:
    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
        bHandled = TRUE;
        return HTCLIENT;
    default:
        return hit;
    }
}

void AppWindow::ApplyDefaultWindowSize() {
    int width = static_cast<int>(backend_.CurrentSettings().main_window_width);
    int height = static_cast<int>(backend_.CurrentSettings().main_window_height);
    width = std::clamp(width, 320, 3840);
    height = std::clamp(height, 220, 2160);
    ::SetWindowPos(m_hWnd, nullptr, 0, 0, width, height,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

std::string AppWindow::IconSourceForItem(const core::LaunchItem& item) const {
    return icon_manager_.ParseItemIconSource(item);
}

std::string AppWindow::GenerateNewGroupName() const {
    int index = 1;
    while (true) {
        const std::string candidate = (index == 1) ? "新建分组" : ("新建分组 " + std::to_string(index));
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
