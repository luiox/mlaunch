#include "settings_window.h"

#include <windowsx.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "edit_focus_helper.h"
#include "logger.h"
#include "utils/string_util.h"

using namespace DuiLib;

namespace {

constexpr int kWindowWidth = 420;
constexpr int kWindowHeight = 330;
constexpr UINT kFocusEditMsg = WM_APP + 0x1B;

std::string TrimCopy(const std::string& value) {
    auto out = value;
    out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) {
        out.pop_back();
    }
    return out;
}

CButtonUI* MakeTextButton(LPCTSTR name, LPCTSTR text, int width = 0) {
    auto* button = new CButtonUI();
    button->SetName(name);
    button->SetText(text);
    if (width > 0) {
        button->SetFixedWidth(width);
    }
    button->SetAttribute(_T("normalbkcolor"), _T("0xFFF0F0F0"));
    button->SetAttribute(_T("hotbkcolor"), _T("0xFFE2E2E2"));
    button->SetAttribute(_T("pushedbkcolor"), _T("0xFFD8D8D8"));
    button->SetAttribute(_T("textcolor"), _T("0xFF1A1A1A"));
    button->SetAttribute(_T("bordercolor"), _T("0xFFC8C8C8"));
    button->SetAttribute(_T("bordersize"), _T("1"));
    return button;
}

CEditUI* MakeInput(LPCTSTR name) {
    auto* input = new CEditUI();
    input->SetName(name);
    input->SetFixedHeight(26);
    input->SetAttribute(_T("bordercolor"), _T("0xFFC0C0C0"));
    input->SetAttribute(_T("bordersize"), _T("1"));
    input->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    input->SetAttribute(_T("textpadding"), _T("6,3,6,3"));
    return input;
}

CLabelUI* MakeFieldLabel(LPCTSTR text, int width) {
    auto* label = new CLabelUI();
    label->SetText(text);
    label->SetFixedWidth(width);
    label->SetTextColor(0xFF1A1A1A);
    label->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    return label;
}

CLabelUI* MakeHint(LPCTSTR text) {
    auto* hint = new CLabelUI();
    hint->SetText(text);
    hint->SetFixedHeight(18);
    hint->SetTextColor(0xFF808080);
    hint->SetFont(1);
    hint->SetTextStyle(DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    return hint;
}

int ParseIntText(const CDuiString& text, bool* ok) {
    const std::string narrow = TrimCopy(launcher::util::WideToUtf8(text.GetData()));
    if (narrow.empty()) {
        *ok = false;
        return 0;
    }
    char* end = nullptr;
    const long value = std::strtol(narrow.c_str(), &end, 10);
    *ok = (end != nullptr && *end == '\0');
    return static_cast<int>(value);
}

} // namespace

SettingsWindow::SettingsWindow(const core::Settings& initial, DoneCallback on_done)
    : on_done_(std::move(on_done)),
      draft_(initial) {
    draft_.hotkey = TrimCopy(draft_.hotkey);
}

LRESULT SettingsWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    m_pm.Init(m_hWnd, GetManagerName(), this);
    m_pm.AddFont(1, _T("微软雅黑"), 12, false, false, false);

    auto* root = new CVerticalLayoutUI();
    root->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    root->SetAttribute(_T("bordercolor"), _T("0xFFB8B8B8"));
    root->SetAttribute(_T("bordersize"), _T("1"));
    root->SetAttribute(_T("inset"), _T("14,12,14,12"));
    root->SetAttribute(_T("childpadding"), _T("8"));

    auto* title = new CLabelUI();
    title->SetText(_T("设置"));
    title->SetFixedHeight(24);
    title->SetTextColor(0xFF1A1A1A);
    title->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    root->Add(title);

    // 热键行
    auto* hotkey_row = new CHorizontalLayoutUI();
    hotkey_row->SetFixedHeight(26);
    hotkey_row->SetAttribute(_T("childpadding"), _T("4"));
    hotkey_row->Add(MakeFieldLabel(_T("全局热键"), 70));
    hotkey_input_ = MakeInput(_T("settings_hotkey_input"));
    hotkey_row->Add(hotkey_input_);
    root->Add(hotkey_row);
    root->Add(MakeHint(_T("示例 Alt+1、Ctrl+Alt+S；留空禁用。修改后立即生效")));

    // 执行后隐藏
    auto* hide_row = new CHorizontalLayoutUI();
    hide_row->SetFixedHeight(26);
    hide_row->SetAttribute(_T("childpadding"), _T("4"));
    hide_row->Add(MakeFieldLabel(_T("执行后隐藏主窗口"), 110));
    hide_toggle_ = MakeTextButton(_T("settings_hide_toggle"), draft_.execute_hide ? _T("开") : _T("关"), 56);
    hide_toggle_->SetTextColor(draft_.execute_hide ? 0xFF1A73E8 : 0xFF909090);
    hide_row->Add(hide_toggle_);
    root->Add(hide_row);

    // 默认窗口宽高
    auto* size_row = new CHorizontalLayoutUI();
    size_row->SetFixedHeight(26);
    size_row->SetAttribute(_T("childpadding"), _T("4"));
    size_row->Add(MakeFieldLabel(_T("默认宽高"), 70));
    width_input_ = MakeInput(_T("settings_width_input"));
    width_input_->SetFixedWidth(70);
    width_input_->SetNumberOnly(true);
    size_row->Add(width_input_);
    height_input_ = MakeInput(_T("settings_height_input"));
    height_input_->SetFixedWidth(70);
    height_input_->SetNumberOnly(true);
    size_row->Add(height_input_);
    root->Add(size_row);
    root->Add(MakeHint(_T("默认窗口尺寸在无已保存布局时生效（320-3840 / 220-2160）")));

    // 分组栏宽度
    auto* panel_row = new CHorizontalLayoutUI();
    panel_row->SetFixedHeight(26);
    panel_row->SetAttribute(_T("childpadding"), _T("4"));
    panel_row->Add(MakeFieldLabel(_T("分组栏宽度"), 70));
    panel_input_ = MakeInput(_T("settings_panel_input"));
    panel_input_->SetFixedWidth(70);
    panel_input_->SetNumberOnly(true);
    panel_row->Add(panel_input_);
    root->Add(panel_row);
    root->Add(MakeHint(_T("分组栏宽度范围 80-600，确定后立即生效")));

    auto* spacer = new CControlUI();
    root->Add(spacer);

    auto* actions = new CHorizontalLayoutUI();
    actions->SetFixedHeight(30);
    actions->SetAttribute(_T("childpadding"), _T("8"));
    actions->SetAttribute(_T("childalign"), _T("right"));
    actions->Add(MakeTextButton(_T("settings_ok"), _T("确定"), 88));
    actions->Add(MakeTextButton(_T("settings_cancel"), _T("取消"), 88));
    root->Add(actions);

    m_pm.AttachDialog(root);
    m_pm.AddNotifier(this);

    hotkey_input_->SetText(launcher::util::Utf8ToWide(draft_.hotkey).c_str());
    width_input_->SetText(std::to_wstring(static_cast<int>(draft_.main_window_width)).c_str());
    height_input_->SetText(std::to_wstring(static_cast<int>(draft_.main_window_height)).c_str());
    panel_input_->SetText(std::to_wstring(static_cast<int>(draft_.group_panel_width)).c_str());

    bHandled = TRUE;
    return 0;
}

void SettingsWindow::CreateAndShow(HWND owner_hwnd) {
    RECT owner_rect{};
    ::GetWindowRect(owner_hwnd, &owner_rect);
    const int owner_cx = owner_rect.right - owner_rect.left;
    const int owner_cy = owner_rect.bottom - owner_rect.top;
    const int x = owner_rect.left + (owner_cx > kWindowWidth ? (owner_cx - kWindowWidth) / 2 : 0);
    const int y = owner_rect.top + (owner_cy > kWindowHeight ? (owner_cy - kWindowHeight) / 2 : 0);
    Create(nullptr, _T("MLaunchSettings"), WS_POPUP | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW, x, y, kWindowWidth, kWindowHeight);
    ::ShowWindow(m_hWnd, SW_SHOW);
    ::SetForegroundWindow(m_hWnd);
    appui::FocusNativeEdit(m_pm, hotkey_input_, m_hWnd);
}

bool SettingsWindow::PointOnEditableControl(POINT pt) const {
    CControlUI* control = m_pm.FindControl(pt);
    if (control == nullptr) {
        return false;
    }
    const CDuiString name = control->GetName();
    if (name == _T("settings_hotkey_input") || name == _T("settings_width_input") ||
        name == _T("settings_height_input") || name == _T("settings_panel_input") ||
        name == _T("settings_hide_toggle")) {
        return true;
    }
    return control->GetInterface(_T("ButtonUI")) != nullptr;
}

void SettingsWindow::Confirm() {
    bool ok = false;

    core::Settings next = draft_;
    next.hotkey = TrimCopy(launcher::util::WideToUtf8(hotkey_input_->GetText().GetData()));

    const int width = ParseIntText(width_input_->GetText(), &ok);
    if (!ok || width < 320 || width > 3840) {
        ::MessageBoxW(m_hWnd, L"默认宽度需为 320-3840 的整数", L"设置", MB_ICONWARNING);
        return;
    }
    const int height = ParseIntText(height_input_->GetText(), &ok);
    if (!ok || height < 220 || height > 2160) {
        ::MessageBoxW(m_hWnd, L"默认高度需为 220-2160 的整数", L"设置", MB_ICONWARNING);
        return;
    }
    const int panel = ParseIntText(panel_input_->GetText(), &ok);
    if (!ok || panel < 80 || panel > 600) {
        ::MessageBoxW(m_hWnd, L"分组栏宽度需为 80-600 的整数", L"设置", MB_ICONWARNING);
        return;
    }

    next.main_window_width = static_cast<double>(width);
    next.main_window_height = static_cast<double>(height);
    next.group_panel_width = static_cast<double>(panel);

    if (on_done_) {
        on_done_(true, next);
    }
    Close();
}

void SettingsWindow::CycleInputFocus() {
    // 不依赖 manager 的 m_pFocus（fork 的 PreMessageHandler 会抢先做
    // SetNextTabControl 把焦点挪到按钮上），用窗口内索引确定性轮换。
    DuiLib::CEditUI* order[] = {hotkey_input_, width_input_, height_input_, panel_input_};
    focus_index_ = (focus_index_ + 1) % 4;
    const HWND native_edit = appui::FocusNativeEdit(m_pm, order[focus_index_], m_hWnd);
    if (native_edit != nullptr) {
        // Windows 惯例：Tab 进入字段时全选现有内容。
        ::SendMessageW(native_edit, EM_SETSEL, 0, -1);
    }
}

LRESULT SettingsWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (uMsg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE) {
        ::PostMessage(m_hWnd, kFocusEditMsg, 0, 0);
    }
    if (uMsg == kFocusEditMsg) {
        focus_index_ = 0;
        appui::FocusNativeEdit(m_pm, hotkey_input_, m_hWnd);
        bHandled = TRUE;
        return 0;
    }
    if (uMsg == appui::kCycleFocusMsg) {
        CycleInputFocus();
        bHandled = TRUE;
        return 0;
    }
    if (uMsg == WM_CHAR && wParam != VK_RETURN && wParam != VK_ESCAPE) {
        HWND native_edit = appui::FocusNativeEdit(m_pm, hotkey_input_, m_hWnd);
        if (native_edit != nullptr) {
            ::SendMessage(native_edit, WM_CHAR, wParam, lParam);
            bHandled = TRUE;
            return 0;
        }
    }
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_TAB) {
            CycleInputFocus();
            bHandled = TRUE;
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            if (on_done_) {
                on_done_(false, draft_);
            }
            Close();
            bHandled = TRUE;
            return 0;
        }
        if (wParam == VK_RETURN) {
            Confirm();
            bHandled = TRUE;
            return 0;
        }
    }
    if (uMsg == WM_LBUTTONDOWN) {
        const POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (!PointOnEditableControl(pt)) {
            ::ReleaseCapture();
            ::PostMessage(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            bHandled = TRUE;
            return 0;
        }
    }

    bHandled = FALSE;
    return 0;
}

void SettingsWindow::Notify(TNotifyUI& msg) {
    if (_tcscmp(msg.sType, DUI_MSGTYPE_RETURN) == 0) {
        Confirm();
        return;
    }
    if (_tcscmp(msg.sType, DUI_MSGTYPE_CLICK) == 0 && msg.pSender != nullptr) {
        const CDuiString name = msg.pSender->GetName();
        if (name == _T("settings_ok")) {
            Confirm();
            return;
        }
        if (name == _T("settings_cancel")) {
            if (on_done_) {
                on_done_(false, draft_);
            }
            Close();
            return;
        }
        if (name == _T("settings_hide_toggle")) {
            draft_.execute_hide = !draft_.execute_hide;
            hide_toggle_->SetText(draft_.execute_hide ? _T("开") : _T("关"));
            hide_toggle_->SetTextColor(draft_.execute_hide ? 0xFF1A73E8 : 0xFF909090);
            m_pm.NeedUpdate();
            return;
        }
    }
}

void SettingsWindow::OnFinalMessage(HWND hWnd) {
    WindowImplBase::OnFinalMessage(hWnd);
    delete this;
}
