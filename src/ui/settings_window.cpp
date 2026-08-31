#include "settings_window.h"

#include "dpi_helper.h"

#include <windowsx.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "edit_focus_helper.h"
#include "logger.h"
#include "ui_controls.h"
#include "utils/string_util.h"

using namespace DuiLib;

namespace {

constexpr int kWindowWidth = 580;
constexpr int kWindowHeight = 400;
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
    // 统一走 appui 工厂（CButtonUI 无状态色属性，必须用 appui::ButtonUI 自绘）。
    return appui::MakeTextButton(name, text, width);
}

CEditUI* MakeInput(LPCTSTR name) {
    auto* input = new CEditUI();
    input->SetName(name);
    input->SetFixedHeight(26);
    input->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
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

// 数值输入实时校验：越界/非整数时边框变红，恢复灰框表示有效。
// 只做视觉反馈不改写文本（改写会打断输入），钳制在 Confirm 统一执行。
void ValidateRangeInput(CEditUI* input, int min_value, int max_value) {
    if (input == nullptr) {
        return;
    }
    bool ok = false;
    const int value = ParseIntText(input->GetText(), &ok);
    const bool valid = ok && value >= min_value && value <= max_value;
    input->SetAttribute(_T("bordercolor"), valid ? _T("0xFFD2D2D2") : _T("0xFFD5303A"));
    input->NeedUpdate();
}

// —— 热键捕获 ——

bool IsModifierKey(WPARAM vk) {
    return vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT || vk == VK_LWIN || vk == VK_RWIN;
}

UINT CollectModifiers() {
    UINT mods = 0;
    if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0) mods |= MOD_CONTROL;
    if ((::GetKeyState(VK_MENU) & 0x8000) != 0) mods |= MOD_ALT;
    if ((::GetKeyState(VK_SHIFT) & 0x8000) != 0) mods |= MOD_SHIFT;
    if (((::GetKeyState(VK_LWIN) & 0x8000) != 0) || ((::GetKeyState(VK_RWIN) & 0x8000) != 0)) mods |= MOD_WIN;
    return mods;
}

std::wstring ModifierPrefix(UINT mods) {
    std::wstring out;
    if (mods & MOD_CONTROL) out += L"Ctrl+";
    if (mods & MOD_SHIFT) out += L"Shift+";
    if (mods & MOD_ALT) out += L"Alt+";
    if (mods & MOD_WIN) out += L"Win+";
    return out;
}

// 与 AppWindow::ParseHotkeyString 的词表保持一致（Ctrl/Alt/Shift/Win + 主键）。
std::wstring VirtualKeyName(WPARAM vk) {
    wchar_t buf[8] = {};
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        return std::wstring(1, static_cast<wchar_t>(vk));
    }
    if (vk >= VK_F1 && vk <= VK_F24) {
        swprintf_s(buf, L"F%d", static_cast<int>(vk - VK_F1 + 1));
        return buf;
    }
    switch (vk) {
    case VK_SPACE: return L"Space";
    case VK_TAB: return L"Tab";
    case VK_ESCAPE: return L"Esc";
    case VK_RETURN: return L"Enter";
    case VK_BACK: return L"Back";
    case VK_PRIOR: return L"PgUp";
    case VK_NEXT: return L"PgDn";
    case VK_HOME: return L"Home";
    case VK_END: return L"End";
    case VK_INSERT: return L"Ins";
    case VK_DELETE: return L"Del";
    case VK_PAUSE: return L"Pause";
    case VK_CAPITAL: return L"CapsLock";
    case VK_NUMLOCK: return L"NumLock";
    case VK_SCROLL: return L"ScrollLock";
    case VK_SNAPSHOT: return L"PrintScreen";
    case VK_OEM_3: return L"`";
    case VK_OEM_MINUS: return L"-";
    case VK_OEM_PLUS: return L"=";
    case VK_OEM_4: return L"[";
    case VK_OEM_6: return L"]";
    case VK_OEM_5: return L"\\";
    case VK_OEM_1: return L";";
    case VK_OEM_7: return L"'";
    case VK_OEM_COMMA: return L",";
    case VK_OEM_PERIOD: return L".";
    case VK_OEM_2: return L"/";
    default: return L"";
    }
}

} // namespace

SettingsWindow::SettingsWindow(const core::Settings& initial, DoneCallback on_done)
    : on_done_(std::move(on_done)),
      draft_(initial) {
    draft_.hotkey = TrimCopy(draft_.hotkey);
}

LRESULT SettingsWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    m_pm.Init(m_hWnd, GetManagerName(), this);
    // 与主窗一致：默认微软雅黑 14（否则回落 DuiLib 内置宋体，字面明显不匹配）。
    m_pm.SetDefaultFont(_T("微软雅黑"), 14, false, false, false, false);
    m_pm.AddFont(1, _T("微软雅黑"), 12, false, false, false);

    // 对齐 VB6 原版设置窗：左侧灰底分类导航（选中项白底），右侧为该分类的配置页。
    auto* root = new CHorizontalLayoutUI();
    root->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    root->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    root->SetAttribute(_T("bordersize"), _T("1"));
    root->SetAttribute(_T("childpadding"), _T("0"));

    auto* nav = new CVerticalLayoutUI();
    nav->SetFixedWidth(120);
    nav->SetAttribute(_T("bkcolor"), _T("0xFFE6E6E6"));
    nav->SetAttribute(_T("childpadding"), _T("0"));

    auto make_nav_button = [&](LPCTSTR name, LPCTSTR text) {
        auto* button = new appui::ButtonUI();
        button->SetName(name);
        button->SetText(text);
        button->SetFixedHeight(36);
        // 选中态走粘滞 active（白底盖住灰导航），悬停浅灰。
        button->SetStateColors(0x00000000, 0xFFDCDCDC, 0xFFFFFFFF);
        button->SetTextColor(0xFF1A1A1A);
        button->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        button->SetAttribute(_T("textpadding"), _T("20,0,8,0"));
        button->SetAttribute(_T("bordercolor"), _T("0x00000000"));
        button->SetAttribute(_T("bordersize"), _T("0"));
        nav->Add(button);
        return button;
    };

    auto* content = new CVerticalLayoutUI();
    content->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    content->SetAttribute(_T("inset"), _T("18,16,18,12"));
    content->SetAttribute(_T("childpadding"), _T("6"));

    // —— 分页内容工厂 ——
    auto make_check_row = [&](CVerticalLayoutUI* page, LPCTSTR name, LPCTSTR text, bool checked) -> appui::CheckBoxUI* {
        auto* row = new CHorizontalLayoutUI();
        row->SetFixedHeight(24);
        row->SetAttribute(_T("childpadding"), _T("4"));
        auto* box = new appui::CheckBoxUI();
        box->SetName(name);
        box->SetText(text);
        box->SetFixedHeight(24);
        box->SetChecked(checked);
        row->Add(box);
        page->Add(row);
        return box;
    };
    auto make_num_row = [&](CVerticalLayoutUI* page, LPCTSTR label_text, CEditUI*& input, LPCTSTR name) {
        auto* row = new CHorizontalLayoutUI();
        row->SetFixedHeight(24);
        row->SetAttribute(_T("childpadding"), _T("4"));
        row->Add(MakeFieldLabel(label_text, 140));
        input = MakeInput(name);
        input->SetFixedWidth(70);
        input->SetNumberOnly(true);
        row->Add(input);
        page->Add(row);
    };

    // —— 行为页 ——
    auto* page_behavior = new CVerticalLayoutUI();
    content->Add(page_behavior);
    hide_check_ = make_check_row(page_behavior, _T("settings_hide_check"), _T("启动条目后最小化窗口（原版行为）"), draft_.execute_hide);
    dblclick_check_ = make_check_row(page_behavior, _T("settings_dblclick_check"), _T("双击启动条目（关闭后单击仅选中）"), draft_.double_click_launch);
    closemin_check_ = make_check_row(page_behavior, _T("settings_closemin_check"), _T("关闭按钮最小化而非退出（退出走菜单）"), draft_.close_minimize);
    lock_check_ = make_check_row(page_behavior, _T("settings_lock_check"), _T("锁定布局（禁用拖动/缩放/重排/宽度滚轮）"), draft_.locked);
    autohide_check_ = make_check_row(page_behavior, _T("settings_autohide_check"), _T("失焦自动隐藏（热键唤回）"), draft_.auto_hide);

    // —— 启动页 ——
    auto* page_startup = new CVerticalLayoutUI();
    page_startup->SetVisible(false);
    content->Add(page_startup);
    autorun_check_ = make_check_row(page_startup, _T("settings_autorun_check"), _T("开机自启（当前用户注册表）"), draft_.autorun);
    starthidden_check_ = make_check_row(page_startup, _T("settings_starthidden_check"), _T("启动时隐藏主窗（热键唤出）"), draft_.start_hidden);
    {
        auto* hotkey_row = new CHorizontalLayoutUI();
        hotkey_row->SetFixedHeight(24);
        hotkey_row->SetAttribute(_T("childpadding"), _T("4"));
        hotkey_row->Add(MakeFieldLabel(_T("全局热键"), 140));
        hotkey_input_ = MakeInput(_T("settings_hotkey_input"));
        hotkey_input_->SetReadOnly(true);
        hotkey_row->Add(hotkey_input_);
        page_startup->Add(hotkey_row);
        page_startup->Add(MakeHint(_T("点击输入框后按下组合键（需含 Ctrl/Alt/Shift/Win）；留空禁用")));
    }

    // —— 窗口页 ——
    auto* page_window = new CVerticalLayoutUI();
    page_window->SetVisible(false);
    content->Add(page_window);
    {
        auto* row = new CHorizontalLayoutUI();
        row->SetFixedHeight(24);
        row->SetAttribute(_T("childpadding"), _T("4"));
        row->Add(MakeFieldLabel(_T("默认宽高"), 140));
        width_input_ = MakeInput(_T("settings_width_input"));
        width_input_->SetFixedWidth(60);
        width_input_->SetNumberOnly(true);
        row->Add(width_input_);
        height_input_ = MakeInput(_T("settings_height_input"));
        height_input_->SetFixedWidth(60);
        height_input_->SetNumberOnly(true);
        row->Add(height_input_);
        page_window->Add(row);
    }
    make_num_row(page_window, _T("分组栏宽度"), panel_input_, _T("settings_panel_input"));
    page_window->Add(MakeHint(_T("窗口尺寸范围 320-3840 / 220-2160；分组栏宽度 80-600")));

    // —— 备份页 ——
    auto* page_backup = new CVerticalLayoutUI();
    page_backup->SetVisible(false);
    content->Add(page_backup);
    make_num_row(page_backup, _T("滚动快照保留份数"), backup_rolling_input_, _T("settings_rolling_input"));
    make_num_row(page_backup, _T("每日快照保留天数"), backup_daily_input_, _T("settings_daily_input"));
    page_backup->Add(MakeHint(_T("保留范围：滚动 2-20 份、每日 3-90 天，越界边框变红并在确定时钳制")));

    auto* spacer = new CControlUI();
    content->Add(spacer);

    auto* actions = new CHorizontalLayoutUI();
    actions->SetFixedHeight(30);
    actions->SetAttribute(_T("childpadding"), _T("8"));
    actions->SetAttribute(_T("childalign"), _T("right"));
    actions->Add(MakeTextButton(_T("settings_ok"), _T("确定"), 88));
    actions->Add(MakeTextButton(_T("settings_cancel"), _T("取消"), 88));
    content->Add(actions);

    root->Add(nav);
    root->Add(content);

    nav_buttons_ = {make_nav_button(_T("settings_nav_0"), _T("行为")),
                    make_nav_button(_T("settings_nav_1"), _T("启动")),
                    make_nav_button(_T("settings_nav_2"), _T("窗口")),
                    make_nav_button(_T("settings_nav_3"), _T("备份"))};
    pages_ = {page_behavior, page_startup, page_window, page_backup};
    nav_buttons_[0]->SetActive(true);

    m_pm.AttachDialog(root);
    m_pm.AddNotifier(this);
    m_pm.AddTranslateAccelerator(this);

    hotkey_input_->SetText(launcher::util::Utf8ToWide(draft_.hotkey).c_str());
    width_input_->SetText(std::to_wstring(static_cast<int>(draft_.main_window_width)).c_str());
    height_input_->SetText(std::to_wstring(static_cast<int>(draft_.main_window_height)).c_str());
    panel_input_->SetText(std::to_wstring(static_cast<int>(draft_.group_panel_width)).c_str());
    backup_rolling_input_->SetText(std::to_wstring(draft_.backup_rolling_count).c_str());
    backup_daily_input_->SetText(std::to_wstring(draft_.backup_daily_days).c_str());

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
    // 窗口按逻辑尺寸创建；对齐真实 DPI 的同时按比例放大并重新居中。
    appui::ScaleDialogToWindowDpi(m_pm, m_hWnd, owner_hwnd);
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
        name == _T("settings_rolling_input") || name == _T("settings_daily_input") ||
        _tcsstr(name, _T("_check")) != nullptr) {
        return true;
    }
    // fork 的 GetInterface 按 DUI_CTR_BUTTON="Button" 匹配，查 "ButtonUI" 永远为空，
    // 会让按钮单击落进拖动分支（Enter/Esc 掩盖了这条路径）。
    return control->GetInterface(_T("Button")) != nullptr;
}

void SettingsWindow::StartHotkeyCapture() {
    capturing_ = true;
    capture_prev_ = hotkey_input_->GetText();
    // 焦点收归顶层窗口：Tab 轮换留下的原生 EDIT 子窗口仍持有键盘焦点时，
    // 按键会进 EDIT 而非顶层消息链，捕获就收不到 KEYDOWN；置顶层焦点后
    // 旧 EDIT 失焦自毁（fork 行为），不影响后续编辑（点击字段会重建）。
    ::SetFocus(m_hWnd);
    hotkey_input_->SetAttribute(_T("bordercolor"), _T("0xFF1A73E8"));
    hotkey_input_->SetText(_T("按下组合键，Esc 取消"));
    m_pm.NeedUpdate();
}

void SettingsWindow::CancelHotkeyCapture() {
    capturing_ = false;
    hotkey_input_->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    hotkey_input_->SetText(capture_prev_.GetData());
    m_pm.NeedUpdate();
}

void SettingsWindow::SelectPage(int index) {
    if (index < 0 || index >= 4 || index == current_page_) {
        return;
    }
    current_page_ = index;
    for (int i = 0; i < 4; ++i) {
        pages_[i]->SetVisible(i == index);
        nav_buttons_[i]->SetActive(i == index);
    }
    m_pm.NeedUpdate();
}

void SettingsWindow::Confirm() {
    bool ok = false;

    // 捕获未完成就点确定：放弃录制，恢复原文本。
    if (capturing_) {
        CancelHotkeyCapture();
    }

    core::Settings next = draft_;
    next.hotkey = TrimCopy(launcher::util::WideToUtf8(hotkey_input_->GetText().GetData()));
    // 控件状态在确认时统一读取；CheckBoxUI::Activate 先通知后翻转，
    // CLICK 通知里读 IsChecked 拿到的是旧值（与其它字段同风格）。
    next.execute_hide = hide_check_->IsChecked();
    next.autorun = autorun_check_->IsChecked();
    next.start_hidden = starthidden_check_->IsChecked();
    next.close_minimize = closemin_check_->IsChecked();
    next.double_click_launch = dblclick_check_->IsChecked();
    next.locked = lock_check_->IsChecked();
    next.auto_hide = autohide_check_->IsChecked();

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
    const int rolling = ParseIntText(backup_rolling_input_->GetText(), &ok);
    if (!ok || rolling < 2 || rolling > 20) {
        ::MessageBoxW(m_hWnd, L"滚动快照份数需为 2-20 的整数", L"设置", MB_ICONWARNING);
        return;
    }
    const int daily = ParseIntText(backup_daily_input_->GetText(), &ok);
    if (!ok || daily < 3 || daily > 90) {
        ::MessageBoxW(m_hWnd, L"每日快照天数需为 3-90 的整数", L"设置", MB_ICONWARNING);
        return;
    }

    next.main_window_width = static_cast<double>(width);
    next.main_window_height = static_cast<double>(height);
    next.group_panel_width = static_cast<double>(panel);
    next.backup_rolling_count = rolling;
    next.backup_daily_days = daily;

    if (on_done_) {
        on_done_(true, next);
    }
    Close();
}

void SettingsWindow::CycleInputFocus() {
    // 不依赖 manager 的 m_pFocus（fork 的 PreMessageHandler 会抢先做
    // SetNextTabControl 把焦点挪到按钮上），用窗口内索引确定性轮换。
    const struct {
        DuiLib::CEditUI* input;
        int page;
    } order[] = {{hotkey_input_, 1}, {width_input_, 2}, {height_input_, 2},
                 {panel_input_, 2}, {backup_rolling_input_, 3}, {backup_daily_input_, 3}};
    // Tab 只在当前页内轮换（其余页的输入框不可见，聚焦会落到空处）。
    for (int step = 0; step < 6; ++step) {
        focus_index_ = (focus_index_ + 1) % 6;
        if (order[focus_index_].page != current_page_) {
            continue;
        }
        const HWND native_edit = appui::FocusNativeEdit(m_pm, order[focus_index_].input, m_hWnd);
        if (native_edit != nullptr) {
            // Windows 惯例：Tab 进入字段时全选现有内容。
            ::SendMessageW(native_edit, EM_SETSEL, 0, -1);
        }
        return;
    }
}

LRESULT SettingsWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (uMsg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE) {
        ::PostMessage(m_hWnd, kFocusEditMsg, 0, 0);
    }
    if (uMsg == kFocusEditMsg) {
        // 热键框已改为只读+点击捕获式：不再自动聚焦它——那会给只读框创建
        // 原生 EDIT 并持有键盘焦点，把捕获态的按键/点击全部拦截在子窗口层。
        // 其余输入框不预聚焦，用户点击或 Tab 时按需创建。
        bHandled = TRUE;
        return 0;
    }
    if (uMsg == appui::kCycleFocusMsg) {
        CycleInputFocus();
        bHandled = TRUE;
        return 0;
    }

    // 热键捕获模式：拦截全部键盘消息用于录制组合键。
    if (capturing_ && (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)) {
        const WPARAM vk = wParam;
        if (vk == VK_ESCAPE) {
            CancelHotkeyCapture();
        } else if (vk == VK_TAB) {
            CancelHotkeyCapture();
            CycleInputFocus();
        } else if (IsModifierKey(vk)) {
            // 只按了修饰键：显示进度提示，继续等待主键。
            hotkey_input_->SetText((ModifierPrefix(CollectModifiers()) + L"…").c_str());
        } else {
            const UINT mods = CollectModifiers();
            const std::wstring key = VirtualKeyName(vk);
            if (mods == 0 || key.empty()) {
                hotkey_input_->SetText(L"需包含 Ctrl/Alt/Shift/Win 修饰键");
            } else {
                hotkey_input_->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
                hotkey_input_->SetText((ModifierPrefix(mods) + key).c_str());
                capturing_ = false;
            }
        }
        m_pm.NeedUpdate();
        bHandled = TRUE;
        return 0;
    }
    if (capturing_ && (uMsg == WM_CHAR || uMsg == WM_SYSCHAR)) {
        // 组合键由 KEYDOWN 路径录制，字符消息全部吞掉（含 Alt+字母的系统菜单路径）。
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
        CControlUI* hit = m_pm.FindControl(pt);
        const bool on_hotkey = (hit != nullptr && hit->GetName() == _T("settings_hotkey_input"));
        if (on_hotkey && !capturing_) {
            StartHotkeyCapture();
            bHandled = TRUE;
            return 0;
        }
        if (capturing_) {
            CancelHotkeyCapture();
            if (on_hotkey) {
                bHandled = TRUE;
                return 0;
            }
            // 点到别处：取消捕获后继续默认处理（可编辑聚焦 / 空白拖拽）。
        }
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
    if (_tcscmp(msg.sType, DUI_MSGTYPE_TEXTCHANGED) == 0 && msg.pSender != nullptr) {
        const CDuiString name = msg.pSender->GetName();
        if (name == _T("settings_width_input")) {
            ValidateRangeInput(width_input_, 320, 3840);
        } else if (name == _T("settings_height_input")) {
            ValidateRangeInput(height_input_, 220, 2160);
        } else if (name == _T("settings_panel_input")) {
            ValidateRangeInput(panel_input_, 80, 600);
        } else if (name == _T("settings_rolling_input")) {
            ValidateRangeInput(backup_rolling_input_, 2, 20);
        } else if (name == _T("settings_daily_input")) {
            ValidateRangeInput(backup_daily_input_, 3, 90);
        }
        return;
    }
    if (_tcscmp(msg.sType, DUI_MSGTYPE_RETURN) == 0) {
        Confirm();
        return;
    }
    if (_tcscmp(msg.sType, DUI_MSGTYPE_CLICK) == 0 && msg.pSender != nullptr) {
        const CDuiString name = msg.pSender->GetName();
        for (int i = 0; i < 4; ++i) {
            if (name == nav_buttons_[i]->GetName()) {
                SelectPage(i);
                return;
            }
        }
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
    }
}

void SettingsWindow::OnFinalMessage(HWND hWnd) {
    m_pm.RemoveTranslateAccelerator(this);
    WindowImplBase::OnFinalMessage(hWnd);
    delete this;
}

LRESULT SettingsWindow::TranslateAccelerator(MSG* pMsg) {
    // fork 的 MessageLoop 在派发前用 PreMessageHandler 吞掉 VK_TAB 做
    // SetNextTabControl（UIManager.cpp），捕获态的"Tab 取消"必须在这层拦截，
    // 否则焦点被挪到下一个输入框、后续按键漏进它的原生 EDIT。
    // 返回值约定（fork UIManager 聚合器：lResult == S_OK 即吞掉）：
    // S_OK=吞掉该消息、S_FALSE=放行。
    if (capturing_ && pMsg != nullptr && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB) {
        CancelHotkeyCapture();
        CycleInputFocus();
        return S_OK;
    }
    return S_FALSE;
}
