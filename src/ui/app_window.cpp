#include "app_window.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>

#include <UIlib.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "constants.h"
#include "dialog_manager.h"
#include "dpi_helper.h"
#include "list_controller.h"
#include "logger.h"
#include "search_controller.h"
#include "ui_builder.h"
#include "utils/string_util.h"

using namespace DuiLib;

// —— app_window 内部辅助（仅 ui 自用，非公共 API）——
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

// ============================================================================
// 构造、初始化与核心流程：OnCreate / 消息分发 / 设置应用 / 对话框转发
// ============================================================================

AppWindow::AppWindow(std::filesystem::path legacy_root)
    : launch_executor_(),
      shortcut_resolver_(),
      backend_(appwin::GetAppBaseDir(), std::move(legacy_root), &launch_executor_, &shortcut_resolver_),
      icon_manager_(appwin::GetAppBaseDir()),
      ui_builder_(*this),
      list_controller_(*this),
      search_controller_(*this),
      dialog_manager_(*this),
      menu_controller_(*this) {
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
        menu_controller_.ShowBackupRecoveryMenu();
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
            menu_controller_.ImportPonerFile(legacy_data_path);
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
        menu_controller_.ExecuteSearchCommand(selected_item_id_);
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

void AppWindow::ToggleSelectedItemEnabled() {
    const core::LaunchItem* item = FindSelectedItem();
    if (item == nullptr) {
        status_.Warn("请先选择条目");
        return;
    }
    const std::string group_id = !selected_item_group_id_.empty() ? selected_item_group_id_ : active_group_id_;
    if (group_id.empty()) {
        status_.Warn("未选中分组");
        return;
    }
    const bool next_enabled = !item->enabled;
    std::string error;
    if (!backend_.SetItemEnabled(group_id, item->id, next_enabled, &error)) {
        status_.Error("切换失败：" + error);
        return;
    }
    RenderItems();
    status_.Info(next_enabled ? "已启用条目" : "已禁用条目（置灰，不可启动）");
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
    // 必须先于 __InitWindow：布局属性读取、字体构建、shell 图标取材都依赖
    // PaintManager 缩放；晚了列表图标会按 100% 取 16px 小图再被放大发虚。
    AlignDpi();
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
    title_label_ = static_cast<CLabelUI*>(m_pm.FindControl(_T("title_label")));
    status_line_ = static_cast<CLabelUI*>(m_pm.FindControl(_T("status_line")));
    search_bar_ = m_pm.FindControl(_T("search_bar"));
    search_button_ = static_cast<appui::IconButtonUI*>(m_pm.FindControl(_T("searchbtn")));
    group_panel_ = static_cast<CVerticalLayoutUI*>(m_pm.FindControl(_T("group_panel")));
    panel_splitter_ = m_pm.FindControl(_T("panel_splitter"));
    search_input_ = static_cast<CEditUI*>(m_pm.FindControl(_T("search_input")));
    group_dialog_ = static_cast<CVerticalLayoutUI*>(m_pm.FindControl(_T("group_dialog")));
    group_dialog_title_ = static_cast<CLabelUI*>(m_pm.FindControl(_T("group_dialog_title")));
    group_dialog_input_ = static_cast<CEditUI*>(m_pm.FindControl(_T("group_dialog_input")));
    group_rename_edit_ = static_cast<CEditUI*>(m_pm.FindControl(_T("group_rename_edit")));
    status_.Bind(status_line_, m_hWnd, launcher::constants::timer::kStatusToast);

    DragAcceptFiles(m_hWnd, TRUE);

    // 先加载数据与设置，RestoreUiState 才能用设置里的默认分组栏宽度。
    LoadBackendData();
    ApplyTitleSetting();
    RestoreUiState();
    RegisterConfiguredHotkey();
    // 锁定/自动隐藏等行为开关随设置生效。
    layout_locked_ = backend_.CurrentSettings().locked;
    auto_hide_ = backend_.CurrentSettings().auto_hide;
    // 开机自启注册表每次启动按设置对齐（exe 挪位后修正路径/清残留）。
    ApplyAutorunRegistry(backend_.CurrentSettings().autorun);

    __InitWindow();

    bHandled = TRUE;
    return 0;
}

void AppWindow::AlignDpi() {
    // 窗口创建时几何已按物理像素摆好（默认尺寸/恢复布局都会换算），这里只对齐
    // 渲染缩放，避免 CPaintManagerUI::SetDPI 内部的比例缩放造成二次放大。
    appui::AlignPaintManagerDpi(m_pm, m_hWnd);
    dpi_aligned_ = true;
}

LRESULT AppWindow::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) {
    if (uMsg == WM_DPICHANGED) {
        if (!dpi_aligned_) {
            // 初始化阶段（恢复布局跨屏移动会触发）：此刻渲染缩放必须保持 100%，
            // 否则 fork 在创建尾声会按 GetMainMonitorDPI(96) 把窗口按比例砍半。
            // 几何由调用方用物理像素摆好，这里直接吞掉。
            bHandled = true;
            return 0;
        }
        // 跨显示器拖动：按系统建议矩形（新 DPI 下的物理几何）摆窗口，再同步
        // 渲染缩放。ui_state 落盘统一存 96 基准逻辑值，SaveUiState 自行换算。
        const auto* suggested = reinterpret_cast<const RECT*>(lParam);
        ::SetWindowPos(m_hWnd, nullptr, suggested->left, suggested->top,
                       suggested->right - suggested->left, suggested->bottom - suggested->top,
                       SWP_NOZORDER | SWP_NOACTIVATE);
        appui::AlignPaintManagerDpi(m_pm, m_hWnd);
        bHandled = true;
        return 0;
    }
    BOOL custom_handled = FALSE;
    const LRESULT custom_result = HandleCustomMessage(uMsg, wParam, lParam, custom_handled);
    if (custom_handled) {
        bHandled = true;
        return custom_result;
    }
    return WindowImplBase::MessageHandler(uMsg, wParam, lParam, bHandled);
}

LRESULT AppWindow::TranslateAccelerator(MSG* pMsg) {
    // 原生 EDIT 子窗口内的键盘消息只到 EDIT（Win32 机制），在 fork 消息循环
    // 派发前拦截；键位→业务动作属宿主职责，不下沉进 DuiLib。
    // 返回约定见头文件：S_OK 吞掉、S_FALSE 放行。
    if (group_rename_active_ && pMsg != nullptr && pMsg->message == WM_KEYDOWN
        && pMsg->hwnd != nullptr && pMsg->hwnd != m_hWnd
        && ::GetAncestor(pMsg->hwnd, GA_ROOT) == m_hWnd) {
        if (pMsg->wParam == VK_ESCAPE) {
            CancelGroupRename();
            return S_OK;
        }
    }

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

    // 分组面板/分隔条上的滚轮 = 调整分组栏宽度（80-600 钳制，ui_state 持久化）。
    // 必须在消息循环层拦截：fork 的 OnMouseWheel 会先把事件发给命中控件
    // （分组列表滚走），HandleCustomMessage 阶段已无法撤回。锁定布局时禁用。
    if (pMsg != nullptr && pMsg->message == WM_MOUSEWHEEL && pMsg->hwnd == m_hWnd
        && !layout_locked_ && !search_mode_
        && group_panel_ != nullptr && panel_splitter_ != nullptr) {
        POINT client{GET_X_LPARAM(pMsg->lParam), GET_Y_LPARAM(pMsg->lParam)};
        ::ScreenToClient(m_hWnd, &client);
        const RECT panel_rect = group_panel_->GetPos();
        const RECT splitter_rect = panel_splitter_->GetPos();
        if (::PtInRect(&panel_rect, client) || ::PtInRect(&splitter_rect, client)) {
            const int delta = (static_cast<short>(HIWORD(pMsg->wParam)) > 0) ? 8 : -8;
            // GetFixedWidth 返回的是缩放后的物理值，SetFixedWidth 存逻辑值；
            // 读-改-写必须先反算回逻辑值，否则非 100% 缩放下每滚一次都翻倍。
            int width = m_pm.GetDPIObj()->ScaleIntBack(group_panel_->GetFixedWidth()) + delta;
            width = std::clamp(width, launcher::constants::layout::kMinGroupPanelWidth,
                               launcher::constants::layout::kMaxGroupPanelWidth);
            group_panel_->SetFixedWidth(width);
            m_pm.NeedUpdate();
            MarkUiStateDirty();
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
            // 关闭即最小化模式：关闭按钮只收进任务栏，退出走主菜单。
            if (backend_.CurrentSettings().close_minimize) {
                ::ShowWindow(m_hWnd, SW_MINIMIZE);
            } else {
                ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
            }
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
            menu_controller_.ShowMainContextMenu(menu_point, true);
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

    // 分组原地重命名：回车提交；失焦（点击别处）也提交，ESC 走 TranslateAccelerator 取消。
    if (msg.pSender != nullptr && msg.pSender->GetName() == _T("group_rename_edit")) {
        if (_tcscmp(msg.sType, DUI_MSGTYPE_RETURN) == 0) {
            CommitGroupRename();
            return;
        }
        if (_tcscmp(msg.sType, DUI_MSGTYPE_KILLFOCUS) == 0 && group_rename_active_) {
            CommitGroupRename();
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
                // 对齐 VB6 行为：左键单击即启动（拖拽重排不会触发 ITEMCLICK）；
                // 双击启动模式下单击仅选中，启动走 ITEMACTIVATE。
                if (!backend_.CurrentSettings().double_click_launch) {
                    LaunchSelectedItem();
                }
            }
            return;
        }
    }

    // 双击启动模式：列表条目双击（ITEMACTIVATE）才触发启动。
    if (_tcscmp(msg.sType, DUI_MSGTYPE_ITEMACTIVATE) == 0 && msg.pSender != nullptr) {
        if (items_list_ != nullptr && appwin::IsSenderFromList(msg.pSender, items_list_)) {
            if (backend_.CurrentSettings().double_click_launch) {
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

void AppWindow::StartGroupRename(const std::string& group_id) {
    if (group_rename_edit_ == nullptr || groups_list_ == nullptr) {
        return;
    }
    if (backend_.IsRecycleBinId(group_id)) {
        status_.Info("回收站由系统自动管理");
        return;
    }

    const core::Group* group = nullptr;
    for (const auto& candidate : backend_.Data().groups) {
        if (candidate.id == group_id) {
            group = &candidate;
            break;
        }
    }
    if (group == nullptr) {
        status_.Warn("分组不存在");
        return;
    }

    int index = -1;
    for (int i = 0; i < static_cast<int>(group_ids_.size()); ++i) {
        if (group_ids_[i] == group_id) {
            index = i;
            break;
        }
    }
    CControlUI* row = index >= 0 ? groups_list_->GetItemAt(index) : nullptr;
    if (row == nullptr) {
        status_.Warn("分组不存在");
        return;
    }

    // float 控件的 rect 由布局的 SetFloatPos 按 FixedXY + FixedWidth/Height 每次
    // 重算（96 基准逻辑值，Get 时按 DPI 缩放），手动 SetPos 会在下一次布局被
    // 重置成 (0,0) 零尺寸——必须走 Fixed 值才能稳定盖在分组行上。
    auto* dpi = m_pm.GetDPIObj();
    const RECT row_rect = row->GetPos();
    group_rename_edit_->SetFixedXY(CDuiSize(dpi->ScaleIntBack(row_rect.left),
                                            dpi->ScaleIntBack(row_rect.top)));
    group_rename_edit_->SetFixedWidth(dpi->ScaleIntBack(row_rect.right - row_rect.left));
    group_rename_edit_->SetFixedHeight(dpi->ScaleIntBack(row_rect.bottom - row_rect.top));
    group_rename_edit_->SetText(launcher::util::Utf8ToWide(group->name).c_str());
    group_rename_edit_->SetVisible(true);
    group_rename_group_id_ = group_id;
    group_rename_active_ = true;
    m_pm.NeedUpdate();
    // 与搜索框同套路：等重排把 rect 摆好后再创建原生 EDIT（同步 SetFocus 会以
    // 旧/0 rect 创建且之后不再跟随重排）。
    ::PostMessage(m_pm.GetPaintWindow(), launcher::constants::kFocusGroupRenameMsg, 0, 0);
}

void AppWindow::CommitGroupRename() {
    if (!group_rename_active_ || group_rename_edit_ == nullptr) {
        return;
    }
    group_rename_active_ = false;
    // 先把焦点还给主窗：关闭仍打开的原生 EDIT，避免焦点落在即将隐藏的控件上。
    ::SetFocus(m_hWnd);

    const std::string trimmed = launcher::util::Trim(
        launcher::util::WideToUtf8(group_rename_edit_->GetText().GetData()));

    const std::string group_id = group_rename_group_id_;
    group_rename_group_id_.clear();
    group_rename_edit_->SetVisible(false);

    if (trimmed.empty()) {
        status_.Warn("分组名不能为空");
        return;
    }

    std::string error;
    if (!backend_.RenameGroup(group_id, trimmed, &error)) {
        status_.Error("重命名分组失败：" + error);
        return;
    }
    status_.Info("分组已重命名");
    RenderGroups();
    for (int i = 0; i < static_cast<int>(group_ids_.size()); ++i) {
        if (group_ids_[i] == group_id) {
            SelectGroupByIndex(i);
            break;
        }
    }
}

void AppWindow::CancelGroupRename() {
    if (!group_rename_active_ || group_rename_edit_ == nullptr) {
        return;
    }
    group_rename_active_ = false;
    ::SetFocus(m_hWnd);
    group_rename_group_id_.clear();
    group_rename_edit_->SetVisible(false);
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
    ApplyTitleSetting();

    layout_locked_ = backend_.CurrentSettings().locked;
    auto_hide_ = backend_.CurrentSettings().auto_hide;

    if (group_panel_ != nullptr) {
        int panel_width = static_cast<int>(backend_.CurrentSettings().group_panel_width);
        panel_width = std::clamp(panel_width, launcher::constants::layout::kMinGroupPanelWidth,
                                 launcher::constants::layout::kMaxGroupPanelWidth);
        group_panel_->SetFixedWidth(panel_width);
    }
    m_pm.NeedUpdate();
}

void AppWindow::ApplyTitleSetting() {
    const std::wstring title = launcher::util::Utf8ToWide(backend_.CurrentSettings().title);
    if (title_label_ != nullptr) {
        title_label_->SetText(title.c_str());
    }
    // OS 窗口文本供任务栏/Alt-Tab/单实例外自动化按标题定位使用。
    ::SetWindowTextW(m_hWnd, title.c_str());
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

bool AppWindow::ShouldStartHidden() const {
    return backend_.CurrentSettings().start_hidden;
}

void AppWindow::ApplyDefaultWindowSize() {
    int width = static_cast<int>(backend_.CurrentSettings().main_window_width);
    int height = static_cast<int>(backend_.CurrentSettings().main_window_height);
    // 钳制在逻辑空间完成（settings 存的是 96 基逻辑值），再按 DPI 放大为物理像素。
    width = std::clamp(width, core::limits::kMainWindowWidthMin, core::limits::kMainWindowWidthMax);
    height = std::clamp(height, core::limits::kMainWindowHeightMin, core::limits::kMainWindowHeightMax);
    const int dpi = static_cast<int>(::GetDpiForWindow(m_hWnd));
    width = ::MulDiv(width, dpi, 96);
    height = ::MulDiv(height, dpi, 96);
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

// ============================================================================
// 窗口生命周期：UI 状态恢复与落盘、全局热键注册与显隐
// ============================================================================

namespace {

// 指定屏幕点的有效 DPI（Shcore 动态加载，失败回退 96）。
int DpiOfPoint(POINT pt) {
    const HMONITOR monitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr) {
        return 96;
    }
    // LoadLibrary 而非 GetModuleHandle：进程未必已加载 Shcore，取句柄失败会
    // 错误地回退 96，恢复的窗口几何就会差一倍。
    if (HMODULE shcore = ::LoadLibraryW(L"Shcore.dll")) {
        using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
        if (auto get_dpi = reinterpret_cast<GetDpiForMonitorFn>(
                ::GetProcAddress(shcore, "GetDpiForMonitor"))) {
            UINT dpi_x = 96;
            UINT dpi_y = 96;
            if (SUCCEEDED(get_dpi(monitor, 0 /*MDT_EFFECTIVE_DPI*/, &dpi_x, &dpi_y)) && dpi_x > 0) {
                return static_cast<int>(dpi_x);
            }
        }
    }
    return 96;
}

} // namespace

void AppWindow::RestoreUiState() {
    const auto ini_path = appwin::GetUiStatePath();

    // 1) 先完成全部读取。未保存过布局时回退到设置里的分组栏宽度。
    int splitter_width = static_cast<int>(backend_.CurrentSettings().group_panel_width);
    const bool has_splitter = group_panel_ != nullptr &&
                              appwin::ReadIniInt(ini_path, L"layout", L"splitter_width", &splitter_width);

    // 兼容两种格式：
    // 1) 老格式：left/top/right/bottom
    // 2) 新格式：left/top/width/height
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    bool has_rect = false;

    const bool has_left = appwin::ReadIniInt(ini_path, L"window", L"left", &left);
    const bool has_top = appwin::ReadIniInt(ini_path, L"window", L"top", &top);
    const bool has_width = appwin::ReadIniInt(ini_path, L"window", L"width", &width);
    const bool has_height = appwin::ReadIniInt(ini_path, L"window", L"height", &height);

    if (has_left && has_top && has_width && has_height) {
        has_rect = true;
    } else {
        int right = 0;
        int bottom = 0;
        if (appwin::ReadIniInt(ini_path, L"window", L"left", &left) &&
            appwin::ReadIniInt(ini_path, L"window", L"top", &top) &&
            appwin::ReadIniInt(ini_path, L"window", L"right", &right) &&
            appwin::ReadIniInt(ini_path, L"window", L"bottom", &bottom)) {
            width = right - left;
            height = bottom - top;
            has_rect = true;
        }
    }

    int maximized = 0;
    const bool has_maximized = appwin::ReadIniInt(ini_path, L"window", L"maximized", &maximized);

    // 2) 读取完毕立即冲刷缓存：GetPrivateProfileStringW 会在进程内缓存该
    //    INI 的句柄，不释放的话，之后 SaveUiState 的原子替换将永远失败。
    ::WritePrivateProfileStringW(nullptr, nullptr, nullptr, ini_path.wstring().c_str());

    // 3) 应用读取到的状态。窗口几何换算：ini 恒存 96 基逻辑值，
    //    DPI 感知进程看到的是物理像素，恢复时按当前窗口 DPI 放大。
    //    （分组栏宽度不换算：fork 对 FixedWidth 在读取时自行 ScaleInt。）
    // 换算 DPI 取“恢复目标位置所在显示器”：创建时 CW_USEDEFAULT 落在哪块屏
    // 并不确定，若用移动前窗口的 DPI，多显示器缩放不同时几何会差一倍。
    const POINT target_center{left + width / 2, top + height / 2};
    const int restore_dpi = DpiOfPoint(target_center);
    left = ::MulDiv(left, restore_dpi, 96);
    top = ::MulDiv(top, restore_dpi, 96);
    width = ::MulDiv(width, restore_dpi, 96);
    height = ::MulDiv(height, restore_dpi, 96);

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
    const auto ini_path = appwin::GetUiStatePath();

    appwin::UiStateSnapshot snapshot{};

    // 先从当前 UI 组装快照（内存为准），再一次性写盘。
    // 窗口几何缩回 96 基逻辑值存储（与恢复侧对称；跨 DPI 迁移时尺寸语义稳定）。
    const int save_dpi = static_cast<int>(::GetDpiForWindow(m_hWnd));
    if (group_panel_ != nullptr) {
        // GetFixedWidth 是缩放后的物理值，ui_state 统一存 96 基准逻辑值。
        int splitter_width = m_pm.GetDPIObj()->ScaleIntBack(group_panel_->GetFixedWidth());
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
    snapshot.left = ::MulDiv(rc.left, 96, save_dpi);
    snapshot.top = ::MulDiv(rc.top, 96, save_dpi);
    snapshot.right = ::MulDiv(rc.right, 96, save_dpi);
    snapshot.bottom = ::MulDiv(rc.bottom, 96, save_dpi);
    snapshot.width = snapshot.right - snapshot.left;
    snapshot.height = snapshot.bottom - snapshot.top;
    snapshot.maximized = placement.showCmd == SW_SHOWMAXIMIZED ? 1 : 0;

    // 事务性写入：先写临时文件，再原子替换正式配置，避免中途损坏。
    if (!appwin::WriteUiStateAtomically(ini_path, snapshot)) {
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

bool AppWindow::ParseHotkeyString(const std::string& text, UINT* modifiers, UINT* virtual_key) {
    if (modifiers == nullptr || virtual_key == nullptr) {
        return false;
    }
    *modifiers = 0;
    *virtual_key = 0;

    // 按 '+' 切分；最后一个 token 是主键，其余必须是修饰键。
    std::vector<std::string> tokens;
    std::string current;
    for (const char ch : text) {
        if (ch == '+') {
            tokens.push_back(current);
            current.clear();
            continue;
        }
        if (ch != ' ' && ch != '\t') {
            current.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }
    tokens.push_back(current);
    if (tokens.size() < 2 || tokens.back().empty()) {
        // 至少一个修饰键 + 主键，避免把普通字母注册成全局热键。
        return false;
    }

    UINT mods = 0;
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        if (token == "CTRL" || token == "CONTROL") {
            mods |= MOD_CONTROL;
        } else if (token == "ALT") {
            mods |= MOD_ALT;
        } else if (token == "SHIFT") {
            mods |= MOD_SHIFT;
        } else if (token == "WIN" || token == "WINDOWS") {
            mods |= MOD_WIN;
        } else {
            return false;
        }
    }

    static const std::map<std::string, UINT> kNamedKeys = {
        {"SPACE", VK_SPACE}, {"TAB", VK_TAB}, {"ESC", VK_ESCAPE}, {"ESCAPE", VK_ESCAPE},
        {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN}, {"BACK", VK_BACK}, {"BACKSPACE", VK_BACK},
        {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT},
        {"PGUP", VK_PRIOR}, {"PGDN", VK_NEXT}, {"HOME", VK_HOME}, {"END", VK_END},
        {"INS", VK_INSERT}, {"INSERT", VK_INSERT}, {"DEL", VK_DELETE}, {"DELETE", VK_DELETE},
        {"PAUSE", VK_PAUSE}, {"CAPSLOCK", VK_CAPITAL}, {"NUMLOCK", VK_NUMLOCK},
        {"SCROLLLOCK", VK_SCROLL}, {"PRINTSCREEN", VK_SNAPSHOT},
        {"`", VK_OEM_3}, {"~", VK_OEM_3}, {"-", VK_OEM_MINUS}, {"=", VK_OEM_PLUS},
        {"[", VK_OEM_4}, {"]", VK_OEM_6}, {"\\", VK_OEM_5}, {";", VK_OEM_1},
        {"'", VK_OEM_7}, {",", VK_OEM_COMMA}, {".", VK_OEM_PERIOD}, {"/", VK_OEM_2},
    };

    const std::string key = tokens.back();
    UINT vk = 0;
    if (key.size() == 1 && key[0] >= 'A' && key[0] <= 'Z') {
        vk = key[0];
    } else if (key.size() == 1 && key[0] >= '0' && key[0] <= '9') {
        vk = key[0];
    } else if (key.size() >= 2 && key[0] == 'F' &&
               key.substr(1).find_first_not_of("0123456789") == std::string::npos) {
        const int fn = std::atoi(key.c_str() + 1);
        if (fn >= 1 && fn <= 24) {
            vk = static_cast<UINT>(VK_F1 + fn - 1);
        }
    } else if (const auto it = kNamedKeys.find(key); it != kNamedKeys.end()) {
        vk = it->second;
    }
    if (vk == 0) {
        return false;
    }

    *modifiers = mods;
    *virtual_key = vk;
    return true;
}

void AppWindow::RegisterConfiguredHotkey() {
    if (hotkey_registered_) {
        ::UnregisterHotKey(m_hWnd, launcher::constants::kAppHotkeyId);
        hotkey_registered_ = false;
    }

    const std::string hotkey_text = backend_.CurrentSettings().hotkey;
    if (hotkey_text.empty()) {
        return;
    }

    UINT mods = 0;
    UINT vk = 0;
    if (!ParseHotkeyString(hotkey_text, &mods, &vk)) {
        launcher::log::Warn("invalid hotkey text: " + hotkey_text);
        status_.Warn("全局热键格式无效：" + hotkey_text);
        return;
    }
    if (!::RegisterHotKey(m_hWnd, launcher::constants::kAppHotkeyId, mods | MOD_NOREPEAT, vk)) {
        launcher::log::Warn("register hotkey failed: " + hotkey_text + " err=" + std::to_string(::GetLastError()));
        status_.Warn("全局热键注册失败（可能被占用）：" + hotkey_text);
        return;
    }
    hotkey_registered_ = true;
}

void AppWindow::ToggleMainWindowVisibility() {
    // 最小化的窗口 IsWindowVisible 仍为真，需先按 iconic 分流，否则热键无法还原。
    if (::IsIconic(m_hWnd)) {
        ::ShowWindow(m_hWnd, SW_RESTORE);
        ::SetForegroundWindow(m_hWnd);
        return;
    }
    if (::IsWindowVisible(m_hWnd)) {
        ::ShowWindow(m_hWnd, SW_HIDE);
        return;
    }
    ::ShowWindow(m_hWnd, SW_SHOW);
    ::SetForegroundWindow(m_hWnd);
}

// ============================================================================
// 交互：自绘消息处理、列表拖拽重排、文件拖放、关闭
// ============================================================================

namespace {

constexpr bool kDragDebugLog = true;
constexpr int kListDragStartThresholdPx = 2;

void DebugLog(const std::string& text) {
    if (!kDragDebugLog) {
        return;
    }
    std::string line = "[drag] " + text + "\n";
    std::fputs(line.c_str(), stderr);
    std::fflush(stderr);
    launcher::log::Debug(line);
}

} // namespace

int AppWindow::HitTestListIndex(CListUI* list, const POINT& client_point) const {
    if (list == nullptr) {
        return -1;
    }
    const RECT list_rect = list->GetPos();
    if (!PtInRect(&list_rect, client_point)) {
        return -1;
    }

    for (int i = 0; i < list->GetCount(); ++i) {
        CControlUI* item = list->GetItemAt(i);
        if (item == nullptr || !item->IsVisible()) {
            continue;
        }
        const RECT row_rect = item->GetPos();
        if (PtInRect(&row_rect, client_point)) {
            return i;
        }
    }
    return -1;
}

void AppWindow::ResetListDragState() {
    if (list_drag_polling_) {
        ::KillTimer(m_hWnd, launcher::constants::timer::kListDragPoll);
        list_drag_polling_ = false;
    }
    drag_list_kind_ = DragListKind::None;
    list_drag_prepared_ = false;
    list_dragging_ = false;
    list_drag_from_index_ = -1;
    list_drag_hover_index_ = -1;
    list_drag_down_point_.x = 0;
    list_drag_down_point_.y = 0;
}

bool AppWindow::CommitListDragReorder() {
    if (list_drag_from_index_ < 0 || list_drag_hover_index_ < 0 || list_drag_from_index_ == list_drag_hover_index_) {
        DebugLog("commit skipped from=" + std::to_string(list_drag_from_index_) + " to=" + std::to_string(list_drag_hover_index_));
        return false;
    }

    std::string error;
    if (drag_list_kind_ == DragListKind::Groups) {
        if (list_drag_from_index_ >= static_cast<int>(group_ids_.size()) || list_drag_hover_index_ >= static_cast<int>(group_ids_.size())) {
            return false;
        }

        const std::string dragged_group_id = group_ids_[list_drag_from_index_];
        const std::string hover_group_id = group_ids_[list_drag_hover_index_];
        if (backend_.IsRecycleBinId(dragged_group_id) || backend_.IsRecycleBinId(hover_group_id)) {
            status_.Warn("回收站位置固定，不可拖动");
            return false;
        }

        DebugLog("reorder groups request id=" + dragged_group_id + " from=" + std::to_string(list_drag_from_index_) + " to=" + std::to_string(list_drag_hover_index_));
        if (!backend_.ReorderGroup(dragged_group_id, list_drag_hover_index_, &error)) {
            status_.Error("分组排序失败：" + error);
            return false;
        }

        RenderGroups();
        SelectGroupByIndex(list_drag_hover_index_);
        m_pm.NeedUpdate();
        DebugLog("commit groups from=" + std::to_string(list_drag_from_index_) + " to=" + std::to_string(list_drag_hover_index_));
        status_.Info("分组已调整顺序");
        return true;
    }

    if (drag_list_kind_ == DragListKind::Items) {
        if (search_mode_) {
            status_.Warn("搜索模式下不可调整条目顺序");
            return false;
        }
        if (list_drag_from_index_ >= static_cast<int>(item_ids_.size()) || list_drag_hover_index_ >= static_cast<int>(item_ids_.size())) {
            return false;
        }
        if (active_group_id_.empty()) {
            return false;
        }

        const std::string dragged_item_id = item_ids_[list_drag_from_index_];
        DebugLog("reorder items request id=" + dragged_item_id + " from=" + std::to_string(list_drag_from_index_) + " to=" + std::to_string(list_drag_hover_index_));
        if (!backend_.ReorderItemInGroup(active_group_id_, dragged_item_id, list_drag_hover_index_, &error)) {
            status_.Error("条目排序失败：" + error);
            return false;
        }

        RenderItems();
        m_pm.NeedUpdate();
        if (items_list_ != nullptr) {
            items_list_->SelectItem(list_drag_hover_index_, false);
        }
        if (list_drag_hover_index_ >= 0 && list_drag_hover_index_ < static_cast<int>(item_ids_.size())) {
            selected_item_id_ = item_ids_[list_drag_hover_index_];
            if (list_drag_hover_index_ < static_cast<int>(item_group_ids_.size())) {
                selected_item_group_id_ = item_group_ids_[list_drag_hover_index_];
            }
        }
        DebugLog("commit items from=" + std::to_string(list_drag_from_index_) + " to=" + std::to_string(list_drag_hover_index_));
        status_.Info("条目已调整顺序");
        return true;
    }

    return false;
}

void AppWindow::HandleFileDrop(HDROP drop_handle) {
    const UINT count = DragQueryFileW(drop_handle, 0xFFFFFFFF, nullptr, 0);
    std::vector<std::string> files;
    files.reserve(count);
    for (UINT index = 0; index < count; ++index) {
        const UINT len = DragQueryFileW(drop_handle, index, nullptr, 0);
        std::wstring path(len + 1, L'\0');
        DragQueryFileW(drop_handle, index, path.data(), len + 1);
        path.resize(len);
        files.push_back(launcher::util::WideToUtf8(path));
    }
    DragFinish(drop_handle);

    if (active_group_id_.empty() && !group_ids_.empty()) {
        active_group_id_ = group_ids_.front();
    }

    if (backend_.IsRecycleBinId(active_group_id_)) {
        status_.Warn("不能拖入回收站");
        return;
    }

    std::string error;
    const auto created = backend_.CreateItemsFromDroppedPaths(active_group_id_, files, &error);
    if (!error.empty()) {
        status_.Error("拖入导入失败：" + error);
    } else if (created > 0) {
        RenderItems();
        status_.Info("已导入条目：" + std::to_string(created));
    } else {
        status_.Warn("没有有效的拖入条目");
    }
}

LRESULT AppWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    auto normalize_point = [&](int x, int y) {
        POINT pt{x, y};
        RECT client{};
        ::GetClientRect(m_hWnd, &client);
        if (!PtInRect(&client, pt)) {
            POINT screen_pt{x, y};
            ::ScreenToClient(m_hWnd, &screen_pt);
            if (PtInRect(&client, screen_pt)) {
                pt = screen_pt;
            }
        }
        return pt;
    };

    if (uMsg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE && auto_hide_
        && ::IsWindowVisible(m_hWnd) && !::IsIconic(m_hWnd)) {
        // 失焦自动隐藏：新激活窗口属于本线程（设置/编辑弹窗、确认框、
        // 原生菜单、文件对话框）时不隐藏，否则交给热键唤回。
        const HWND activating = reinterpret_cast<HWND>(lParam);
        const DWORD activating_tid = activating != nullptr ? ::GetWindowThreadProcessId(activating, nullptr) : 0;
        if (activating_tid != ::GetCurrentThreadId()) {
            ::ShowWindow(m_hWnd, SW_HIDE);
        }
    }

    if (uMsg == WM_HOTKEY && wParam == launcher::constants::kAppHotkeyId) {
        ToggleMainWindowVisibility();
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == launcher::constants::kFocusSearchMsg) {
        // 布局完成后再聚焦搜索输入框（见 kFocusSearchMsg 注释）。
        // PostMessage 排在 WM_PAINT 之前被处理（WM_PAINT 优先级最低），
        // 到这里 rect 可能仍是 0,0,0,0——先 UpdateWindow 同步走一次
        // OnPaint 全量重排（root 已被 NeedUpdate 标记），再创建原生 EDIT。
        ::UpdateWindow(m_hWnd);
        if (search_input_ != nullptr) {
            search_input_->SetFocus();
            const int text_len = search_input_->GetText().GetLength();
            search_input_->SetSel(text_len, text_len);
        }
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == launcher::constants::kFocusGroupDialogMsg) {
        // 分组对话框输入框延迟聚焦：先 UpdateWindow 让对话框 rect 算出来
        //（float 控件靠重排定位），再创建原生 EDIT，见 kFocusGroupDialogMsg 注释。
        ::UpdateWindow(m_hWnd);
        if (group_dialog_ != nullptr && group_dialog_->IsVisible() && group_dialog_input_ != nullptr) {
            group_dialog_input_->SetFocus();
            const int text_len = group_dialog_input_->GetText().GetLength();
            group_dialog_input_->SetSel(text_len, text_len);
        }
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == launcher::constants::kFocusGroupRenameMsg) {
        // 分组原地重命名编辑框延迟聚焦：等行 rect 摆好后再创建原生 EDIT。
        ::UpdateWindow(m_hWnd);
        if (group_rename_active_ && group_rename_edit_ != nullptr) {
            group_rename_edit_->SetFocus();
            // CEditUI::SetSel 在此 fork 是空实现，全选现名走原生 EM_SETSEL，
            // 直接输入即覆盖（对齐常规重命名交互）。
            const HWND focused = ::GetFocus();
            if (focused != nullptr) {
                ::SendMessageW(focused, EM_SETSEL, 0, -1);
            }
        }
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_GETMINMAXINFO) {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        if (info != nullptr) {
            // 最小尺寸存 96 基准逻辑值，按当前渲染 DPI 换算成物理像素钳制。
            const int min_w = m_pm.GetDPIObj()->ScaleInt(launcher::constants::layout::kMinWindowWidth);
            const int min_h = m_pm.GetDPIObj()->ScaleInt(launcher::constants::layout::kMinWindowHeight);
            info->ptMinTrackSize.x = min_w;
            info->ptMinTrackSize.y = min_h;
            bHandled = TRUE;
            return 0;
        }
    }

    // 分隔条是透明窄条，悬停时给出左右调整光标作为可拖拽的提示。
    if (uMsg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT
        && !layout_locked_ && panel_splitter_ != nullptr) {
        const DWORD cursor_pos = ::GetMessagePos();
        POINT pt{static_cast<short>(LOWORD(cursor_pos)), static_cast<short>(HIWORD(cursor_pos))};
        ::ScreenToClient(m_hWnd, &pt);
        RECT rc = panel_splitter_->GetPos();
        rc.left -= 3;
        rc.right += 3;
        if (::PtInRect(&rc, pt)) {
            ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEWE));
            bHandled = TRUE;
            return TRUE;
        }
    }

    if (uMsg == WM_LBUTTONDOWN) {
        const int x = static_cast<short>(LOWORD(lParam));
        const int y = static_cast<short>(HIWORD(lParam));
        const POINT pt = normalize_point(x, y);

        // 先初始化拖拽状态，再根据鼠标命中区域决定是否进入列表拖拽准备态。
        ResetListDragState();
        // 锁定布局：禁用列表拖拽重排（分组/条目）。
        if (!layout_locked_) {
            const int group_index = HitTestListIndex(groups_list_, pt);
            if (group_index >= 0) {
                drag_list_kind_ = DragListKind::Groups;
                list_drag_prepared_ = true;
                list_drag_down_point_ = pt;
                list_drag_from_index_ = group_index;
                list_drag_hover_index_ = group_index;
                SetCapture(m_hWnd);
                ::SetTimer(m_hWnd, launcher::constants::timer::kListDragPoll, 16, nullptr);
                list_drag_polling_ = true;
            } else {
                const int item_index = HitTestListIndex(items_list_, pt);
                if (item_index >= 0 && !search_mode_) {
                    drag_list_kind_ = DragListKind::Items;
                    list_drag_prepared_ = true;
                    list_drag_down_point_ = pt;
                    list_drag_from_index_ = item_index;
                    list_drag_hover_index_ = item_index;
                    SetCapture(m_hWnd);
                    ::SetTimer(m_hWnd, launcher::constants::timer::kListDragPoll, 16, nullptr);
                    list_drag_polling_ = true;
                }
            }
        }

        DebugLog("down g=" + std::to_string(HitTestListIndex(groups_list_, pt)) + " i=" + std::to_string(HitTestListIndex(items_list_, pt)));

        // 优先命中分隔条，避免与列表拖拽冲突；锁定布局时禁用分隔条拖动。
        if (!layout_locked_ && panel_splitter_ != nullptr && group_panel_ != nullptr) {
            RECT rc = panel_splitter_->GetPos();
            rc.left -= 3;
            rc.right += 3;
            if (PtInRect(&rc, pt)) {
                ResetListDragState();
                splitter_dragging_ = true;
                splitter_drag_start_x_ = x;
                // GetFixedWidth 是缩放后的物理值，反算回逻辑值再参与拖拽运算，
                // SetFixedWidth 存逻辑值，直接混用会在非 100% 缩放下翻倍。
                splitter_start_width_ = m_pm.GetDPIObj()->ScaleIntBack(group_panel_->GetFixedWidth());
                splitter_pending_width_ = splitter_start_width_;
                splitter_last_update_tick_ = ::GetTickCount();
                SetCapture(m_hWnd);
                bHandled = TRUE;
                return 0;
            }
        }
    }

    if (uMsg == WM_MOUSEMOVE && list_drag_prepared_ && !list_dragging_) {
        const int x = static_cast<short>(LOWORD(lParam));
        const int y = static_cast<short>(HIWORD(lParam));
        const POINT pt = normalize_point(x, y);
        const int dx = std::abs(pt.x - list_drag_down_point_.x);
        const int dy = std::abs(pt.y - list_drag_down_point_.y);
        if ((wParam & MK_LBUTTON) == 0) {
            DebugLog("cancel prepared: lbutton released");
            ReleaseCapture();
            ResetListDragState();
            bHandled = TRUE;
            return 0;
        }
        if (dx >= kListDragStartThresholdPx || dy >= kListDragStartThresholdPx) {
            list_dragging_ = true;
            DebugLog("start drag from=" + std::to_string(list_drag_from_index_));
        }
    }

    if (uMsg == WM_MOUSEMOVE && list_dragging_) {
        const POINT pt = normalize_point(static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)));
        CListUI* target_list = nullptr;
        if (drag_list_kind_ == DragListKind::Groups) {
            target_list = groups_list_;
        } else if (drag_list_kind_ == DragListKind::Items) {
            target_list = items_list_;
        }

        const int hover_index = HitTestListIndex(target_list, pt);
        if (hover_index >= 0 && hover_index != list_drag_hover_index_) {
            list_drag_hover_index_ = hover_index;
            if (target_list != nullptr) {
                target_list->SelectItem(hover_index, false);
            }
            DebugLog("hover -> " + std::to_string(hover_index));
        }
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_MOUSEMOVE && splitter_dragging_ && group_panel_ != nullptr) {
        const int x = static_cast<short>(LOWORD(lParam));
        auto* dpi = m_pm.GetDPIObj();
        int next_width = splitter_start_width_ + dpi->ScaleIntBack(x - splitter_drag_start_x_);

        RECT client{};
        ::GetClientRect(m_hWnd, &client);
        const int total_width = dpi->ScaleIntBack(client.right - client.left);
        const int min_group = launcher::constants::layout::kMinGroupPanelWidth;
        const int min_items = launcher::constants::layout::kMinItemsPanelWidth;
        const int max_group = (total_width - min_items - 12 > min_group) ? (total_width - min_items - 12) : min_group;

        if (next_width < min_group) {
            next_width = min_group;
        }
        if (next_width > max_group) {
            next_width = max_group;
        }

        if (splitter_pending_width_ != next_width) {
            splitter_pending_width_ = next_width;
        }

        // 宽度更新做节流，减少高频拖动导致的重绘压力。
        const DWORD now = ::GetTickCount();
        const bool time_ready = (now - splitter_last_update_tick_) >= 12;
        const int current_width = m_pm.GetDPIObj()->ScaleIntBack(group_panel_->GetFixedWidth());
        const bool delta_large = std::abs(current_width - splitter_pending_width_) >= 3;
        if ((time_ready || delta_large) && current_width != splitter_pending_width_) {
            group_panel_->SetFixedWidth(splitter_pending_width_);
            m_pm.NeedUpdate();
            splitter_last_update_tick_ = now;
        }
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_LBUTTONUP && splitter_dragging_) {
        splitter_dragging_ = false;
        ReleaseCapture();
        if (group_panel_ != nullptr && splitter_pending_width_ >= 0
            && m_pm.GetDPIObj()->ScaleIntBack(group_panel_->GetFixedWidth()) != splitter_pending_width_) {
            group_panel_->SetFixedWidth(splitter_pending_width_);
            m_pm.NeedUpdate();
        }
        MarkUiStateDirty();
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_LBUTTONUP && list_dragging_) {
        ReleaseCapture();
        CommitListDragReorder();
        ResetListDragState();
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_LBUTTONUP && list_drag_prepared_) {
        ReleaseCapture();
        DebugLog("prepared only, no drag");
        ResetListDragState();
    }

    if (uMsg == WM_CAPTURECHANGED && splitter_dragging_) {
        splitter_dragging_ = false;
        if (group_panel_ != nullptr && splitter_pending_width_ >= 0
            && m_pm.GetDPIObj()->ScaleIntBack(group_panel_->GetFixedWidth()) != splitter_pending_width_) {
            group_panel_->SetFixedWidth(splitter_pending_width_);
            m_pm.NeedUpdate();
        }
        MarkUiStateDirty();
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_CAPTURECHANGED && (list_dragging_ || list_drag_prepared_)) {
        const bool left_down = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (!left_down) {
            if (list_dragging_) {
                CommitListDragReorder();
            }
            DebugLog("capture changed -> finalize drag state");
            ResetListDragState();
            bHandled = TRUE;
            return 0;
        }
        DebugLog("capture changed while holding left button, keep dragging");
    }

    // WM_ENTRYSIZEMOVE(0x0231)/WM_EXITSIZEMOVE(0x0232)：部分 SDK 头未暴露，用字面量。
    if (uMsg == 0x0231) {
        // 对齐原版：拖拽/缩放期间挂起重绘（DWM 显示旧帧），结束后统一重排去闪烁。
        m_pm.LockUpdate(true);
    }

    if (uMsg == 0x0232) {
        if (m_pm.IsLockUpdate()) {
            m_pm.LockUpdate(false);
            if (m_pm.GetRoot() != nullptr) {
                m_pm.GetRoot()->NeedUpdate();
            }
            m_pm.NeedUpdate();
            m_pm.Invalidate();
        }
        MarkUiStateDirty();
    }

    if (uMsg == WM_SIZE) {
        m_pm.NeedUpdate();
        if (wParam != SIZE_MINIMIZED) {
            MarkUiStateDirty();
        }
    }

    if (uMsg == WM_MOVE) {
        m_pm.NeedUpdate();
        MarkUiStateDirty();
    }

    if (uMsg == WM_TIMER && wParam == launcher::constants::timer::kStatusToast) {
        status_.Hide();
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_TIMER && wParam == launcher::constants::timer::kUiStateSave) {
        ::KillTimer(m_hWnd, launcher::constants::timer::kUiStateSave);
        ui_state_timer_active_ = false;
        FlushUiStateIfDirty();
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_TIMER && wParam == launcher::constants::timer::kListDragPoll) {
        if (!(list_drag_prepared_ || list_dragging_)) {
            ::KillTimer(m_hWnd, launcher::constants::timer::kListDragPoll);
            list_drag_polling_ = false;
            bHandled = TRUE;
            return 0;
        }

        POINT pt_screen{};
        ::GetCursorPos(&pt_screen);
        POINT pt = pt_screen;
        ::ScreenToClient(m_hWnd, &pt);

        const bool left_down = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (!left_down) {
            if (list_dragging_) {
                CommitListDragReorder();
            }
            ReleaseCapture();
            DebugLog("poll release");
            ResetListDragState();
            bHandled = TRUE;
            return 0;
        }

        if (list_drag_prepared_ && !list_dragging_) {
            const int dx = std::abs(pt.x - list_drag_down_point_.x);
            const int dy = std::abs(pt.y - list_drag_down_point_.y);
            if (dx >= kListDragStartThresholdPx || dy >= kListDragStartThresholdPx) {
                list_dragging_ = true;
                DebugLog("start drag(from poll) from=" + std::to_string(list_drag_from_index_));
            }
        }

        if (list_dragging_) {
            CListUI* target_list = nullptr;
            if (drag_list_kind_ == DragListKind::Groups) {
                target_list = groups_list_;
            } else if (drag_list_kind_ == DragListKind::Items) {
                target_list = items_list_;
            }

            const int hover_index = HitTestListIndex(target_list, pt);
            if (hover_index >= 0 && hover_index != list_drag_hover_index_) {
                list_drag_hover_index_ = hover_index;
                if (target_list != nullptr) {
                    target_list->SelectItem(hover_index, false);
                }
                DebugLog("hover(from poll) -> " + std::to_string(hover_index));
            }
        }

        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_CONTEXTMENU) {
        POINT screen_point{};
        if (lParam == static_cast<LPARAM>(-1)) {
            GetCursorPos(&screen_point);
        } else {
            screen_point.x = GET_X_LPARAM(lParam);
            screen_point.y = GET_Y_LPARAM(lParam);
        }
        POINT client_point = screen_point;
        ScreenToClient(m_hWnd, &client_point);

        const bool over_group = SelectListRowFromPoint(groups_list_, group_ids_, client_point, &active_group_id_);
        if (over_group) {
            RenderItems();
            menu_controller_.ShowGroupContextMenu(screen_point);
            bHandled = TRUE;
            return 0;
        }

        const bool over_item = SelectListRowFromPoint(items_list_, item_ids_, client_point, &selected_item_id_);
        if (over_item) {
            const int index = items_list_ != nullptr ? items_list_->GetCurSel() : -1;
            if (index >= 0 && index < static_cast<int>(item_group_ids_.size())) {
                selected_item_group_id_ = item_group_ids_[index];
            }
            menu_controller_.ShowItemContextMenu(screen_point);
            bHandled = TRUE;
            return 0;
        }

        // 非列表区域（标题栏/空白处）：打开主菜单。
        menu_controller_.ShowMainContextMenu(screen_point);
        bHandled = TRUE;
        return 0;
    }

    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_APPS ||
            (wParam == 'M' && (::GetKeyState(VK_CONTROL) & 0x8000) != 0)) {
            // 键盘菜单入口：与右键同一条路由（光标处命中分组/条目/主菜单）。
            // 部分 DuiLib fork 会吞掉 Shift+F10 的 DefWindowProc 转换，这里主动补发。
            ::PostMessage(m_hWnd, WM_CONTEXTMENU, reinterpret_cast<WPARAM>(m_hWnd), static_cast<LPARAM>(-1));
            bHandled = TRUE;
            return 0;
        }
        if (group_dialog_ != nullptr && group_dialog_->IsVisible()) {
            if (wParam == VK_RETURN) {
                ConfirmGroupDialog();
                bHandled = TRUE;
                return 0;
            }
            if (wParam == VK_ESCAPE) {
                CloseGroupDialog();
                bHandled = TRUE;
                return 0;
            }
        }
        if (wParam == VK_RETURN) {
            LaunchSelectedItem();
            bHandled = TRUE;
            return 0;
        }
        if (wParam == 'Z' && (::GetKeyState(VK_CONTROL) & 0x8000) != 0) {
            UndoLastDelete();
            bHandled = TRUE;
            return 0;
        }
        if (wParam == VK_DELETE) {
            DeleteSelectedItem();
            bHandled = TRUE;
            return 0;
        }
        if (wParam == VK_ESCAPE && search_mode_) {
            search_controller_.ToggleSearchMode();
            bHandled = TRUE;
            return 0;
        }
        // 主窗自身持焦时的搜索导航（EDIT 持焦路径走 TranslateAccelerator 拦截）。
        if (search_mode_ && (wParam == VK_UP || wParam == VK_DOWN)) {
            search_controller_.MoveSearchSelection(wParam == VK_UP ? -1 : 1);
            bHandled = TRUE;
            return 0;
        }
    }

    if (uMsg == WM_DROPFILES) {
        HandleFileDrop(reinterpret_cast<HDROP>(wParam));
        bHandled = TRUE;
        return 0;
    }

    bHandled = FALSE;
    return 0;
}

LRESULT AppWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    m_pm.RemoveTranslateAccelerator(this);
    if (splitter_dragging_) {
        splitter_dragging_ = false;
        ReleaseCapture();
    }
    if (hotkey_registered_) {
        ::UnregisterHotKey(m_hWnd, launcher::constants::kAppHotkeyId);
        hotkey_registered_ = false;
    }
    if (ui_state_timer_active_) {
        ::KillTimer(m_hWnd, launcher::constants::timer::kUiStateSave);
        ui_state_timer_active_ = false;
    }
    ::KillTimer(m_hWnd, launcher::constants::timer::kStatusToast);
    CloseSettingsDialog();
    CloseItemDialog();
    SaveUiState();
    PostQuitMessage(0);
    bHandled = FALSE;
    return 0;
}

// ============================================================================
// 菜单与 Shell 集成：备份恢复/主菜单/分组/条目菜单、导入导出、系统预设项
// ============================================================================

