#include "item_edit_window.h"

#include <windowsx.h>

#include <algorithm>
#include <cctype>

#include "app_window.h"
#include "edit_focus_helper.h"
#include "logger.h"
#include "shell_services.h"
#include "utils/string_util.h"

using namespace DuiLib;

namespace {

constexpr int kWindowWidth = 400;
constexpr int kWindowHeight = 300;
constexpr UINT kFocusEditMsg = WM_APP + 0x1A;

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
    // 与主窗 group_dialog 按钮同体系（E6E6E6/D5D5D5，无边框）。
    button->SetAttribute(_T("normalbkcolor"), _T("0xFFE6E6E6"));
    button->SetAttribute(_T("hotbkcolor"), _T("0xFFD5D5D5"));
    button->SetAttribute(_T("pushedbkcolor"), _T("0xFFD5D5D5"));
    button->SetAttribute(_T("textcolor"), _T("0xFF1A1A1A"));
    button->SetAttribute(_T("bordercolor"), _T("0x00000000"));
    button->SetAttribute(_T("bordersize"), _T("0"));
    return button;
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

// 此 fork 的 CEditUI 依赖原生 EDIT 子窗口接收输入，但 UIEVENT_SETFOCUS
// 路径创建不可靠；真实可用的创建路径是 UIEVENT_BUTTONDOWN（鼠标点击）。
// 键盘自动化/程序化打开时没有点击，需手动补齐 —— 见 edit_focus_helper.h
// 的 appui::FocusNativeEdit（同时子类化原生 EDIT，补上 Tab 轮换与 Ctrl+A）。

CLabelUI* MakeFieldLabel(LPCTSTR text) {
    auto* label = new CLabelUI();
    label->SetText(text);
    label->SetFixedWidth(38);
    label->SetTextColor(0xFF1A1A1A);
    label->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    return label;
}

} // namespace

ItemEditWindow::ItemEditWindow(AppWindow& owner,
                               bool edit_mode,
                               const std::string& group_id,
                               const std::string& item_id,
                               const core::LaunchItem* initial,
                               DoneCallback on_done)
    : owner_(owner),
      on_done_(std::move(on_done)),
      edit_mode_(edit_mode),
      item_id_(item_id) {
    if (initial != nullptr) {
        initial_name_ = initial->name;
        initial_target_ = initial->target_path;
        initial_args_ = initial->arguments;
        icon_location_ = initial->icon_location;
    }
}

LRESULT ItemEditWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    m_pm.Init(m_hWnd, GetManagerName(), this);
    // 与主窗一致：默认微软雅黑 14（否则回落 DuiLib 内置宋体，字面明显不匹配）。
    m_pm.SetDefaultFont(_T("微软雅黑"), 14, false, false, false, false);
    m_pm.AddFont(1, _T("微软雅黑"), 12, false, false, false);

    auto* root = new CVerticalLayoutUI();
    root->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    root->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    root->SetAttribute(_T("bordersize"), _T("1"));
    root->SetAttribute(_T("inset"), _T("14,12,14,12"));
    root->SetAttribute(_T("childpadding"), _T("8"));

    // 顶部：左列 名称输入，右列 图标预览 + 图标按钮
    auto* top_row = new CHorizontalLayoutUI();
    top_row->SetAttribute(_T("childpadding"), _T("8"));

    auto* left_col = new CVerticalLayoutUI();
    auto* name_row = new CHorizontalLayoutUI();
    name_row->SetFixedHeight(26);
    name_row->Add(MakeFieldLabel(_T("名称")));
    name_input_ = MakeInput(_T("item_dialog_name_input"));
    name_row->Add(name_input_);
    left_col->Add(name_row);
    top_row->Add(left_col);

    auto* right_col = new CVerticalLayoutUI();
    right_col->SetFixedWidth(112);
    right_col->SetAttribute(_T("childpadding"), _T("4"));
    right_col->SetAttribute(_T("childalign"), _T("center"));
    icon_preview_ = new FileIconControl();
    icon_preview_->SetName(_T("item_dialog_icon_preview"));
    icon_preview_->SetFixedWidth(48);
    icon_preview_->SetFixedHeight(48);
    right_col->Add(icon_preview_);
    auto* icon_btn_row = new CHorizontalLayoutUI();
    icon_btn_row->SetFixedHeight(24);
    icon_btn_row->SetAttribute(_T("childpadding"), _T("4"));
    icon_btn_row->Add(MakeTextButton(_T("item_dialog_icon_default"), _T("默认图标")));
    icon_btn_row->Add(MakeTextButton(_T("item_dialog_icon_change"), _T("修改图标")));
    right_col->Add(icon_btn_row);
    top_row->Add(right_col);
    root->Add(top_row);

    // 目标行
    auto* target_row = new CHorizontalLayoutUI();
    target_row->SetFixedHeight(26);
    target_row->SetAttribute(_T("childpadding"), _T("4"));
    target_row->Add(MakeFieldLabel(_T("目标")));
    target_input_ = MakeInput(_T("item_dialog_target_input"));
    target_row->Add(target_input_);
    target_row->Add(MakeTextButton(_T("item_dialog_browse"), _T("..."), 30));
    root->Add(target_row);

    // 参数行
    auto* args_row = new CHorizontalLayoutUI();
    args_row->SetFixedHeight(26);
    args_row->SetAttribute(_T("childpadding"), _T("4"));
    args_row->Add(MakeFieldLabel(_T("参数")));
    args_input_ = MakeInput(_T("item_dialog_args_input"));
    args_row->Add(args_input_);
    root->Add(args_row);

    // 灰字提示
    auto* hint1 = new CLabelUI();
    hint1->SetText(_T("目标可以指向文件、文件夹、网址。支持相对路径与环境变量。"));
    hint1->SetFixedHeight(18);
    hint1->SetTextColor(0xFF808080);
    hint1->SetFont(1);
    hint1->SetTextStyle(DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    root->Add(hint1);
    auto* hint2 = new CLabelUI();
    hint2->SetText(_T("%pr%: 程序所在目录    %cr%: 程序所在盘的根目录"));
    hint2->SetFixedHeight(18);
    hint2->SetTextColor(0xFF808080);
    hint2->SetFont(1);
    hint2->SetTextStyle(DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    root->Add(hint2);

    // 底部按钮
    auto* actions = new CHorizontalLayoutUI();
    actions->SetFixedHeight(30);
    actions->SetAttribute(_T("childpadding"), _T("8"));
    actions->SetAttribute(_T("childalign"), _T("right"));
    actions->Add(MakeTextButton(_T("item_dialog_ok"), _T("确定"), 88));
    actions->Add(MakeTextButton(_T("item_dialog_cancel"), _T("取消"), 88));
    root->Add(actions);

    m_pm.AttachDialog(root);
    m_pm.AddNotifier(this);

    name_input_->SetText(launcher::util::Utf8ToWide(initial_name_).c_str());
    target_input_->SetText(launcher::util::Utf8ToWide(initial_target_).c_str());
    args_input_->SetText(launcher::util::Utf8ToWide(initial_args_).c_str());
    RefreshIcon();
    name_input_->SetFocus();

    bHandled = TRUE;
    return 0;
}

void ItemEditWindow::CreateAndShow(HWND owner_hwnd) {
    RECT owner_rect{};
    ::GetWindowRect(owner_hwnd, &owner_rect);
    const int owner_cx = owner_rect.right - owner_rect.left;
    const int owner_cy = owner_rect.bottom - owner_rect.top;
    const int x = owner_rect.left + (owner_cx > kWindowWidth ? (owner_cx - kWindowWidth) / 2 : 0);
    const int y = owner_rect.top + (owner_cy > kWindowHeight ? (owner_cy - kWindowHeight) / 2 : 0);
    Create(nullptr, _T("MLaunchItemEdit"), WS_POPUP | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW, x, y, kWindowWidth, kWindowHeight);
    ::ShowWindow(m_hWnd, SW_SHOW);
    ::SetForegroundWindow(m_hWnd);
    name_input_->SetFocus();
    appui::FocusNativeEdit(m_pm, name_input_, m_hWnd);
}

void ItemEditWindow::RefreshIcon() {
    if (icon_preview_ == nullptr || target_input_ == nullptr) {
        return;
    }
    core::LaunchItem probe;
    probe.item_type = "app";
    probe.target_path = launcher::util::WideToUtf8(target_input_->GetText().GetData());
    probe.icon_location = icon_location_;
    icon_preview_->SetIconPath(launcher::util::Utf8ToWide(owner_.IconSourceForItem(probe)));
    m_pm.NeedUpdate();
}bool ItemEditWindow::PointOnEditableControl(POINT pt) const {
    CControlUI* control = m_pm.FindControl(pt);
    if (control == nullptr) {
        return false;
    }
    const CDuiString name = control->GetName();
    if (name == _T("item_dialog_name_input") || name == _T("item_dialog_target_input") ||
        name == _T("item_dialog_args_input") || name == _T("item_dialog_icon_preview")) {
        return true;
    }
    return control->GetInterface(_T("ButtonUI")) != nullptr;
}

void ItemEditWindow::Confirm() {
    const std::string name = TrimCopy(launcher::util::WideToUtf8(name_input_->GetText().GetData()));
    const std::string target = TrimCopy(launcher::util::WideToUtf8(target_input_->GetText().GetData()));
    const std::string args = launcher::util::WideToUtf8(args_input_->GetText().GetData());

    if (name.empty()) {
        ::MessageBoxW(m_hWnd, L"名称不能为空", L"MLaunch", MB_ICONWARNING);
        return;
    }
    if (target.empty()) {
        ::MessageBoxW(m_hWnd, L"目标不能为空", L"MLaunch", MB_ICONWARNING);
        return;
    }

    if (on_done_) {
        on_done_(true, item_id_, name, target, args, icon_location_);
    }
    Close();
}

void ItemEditWindow::CycleInputFocus() {
    // 不依赖 manager 的 m_pFocus（PreMessageHandler 会抢先 SetNextTabControl），
    // 用窗口内索引确定性轮换：名称 → 目标 → 参数。
    DuiLib::CEditUI* order[] = {name_input_, target_input_, args_input_};
    focus_index_ = (focus_index_ + 1) % 3;
    const HWND native_edit = appui::FocusNativeEdit(m_pm, order[focus_index_], m_hWnd);
    if (native_edit != nullptr) {
        // Windows 惯例：Tab 进入字段时全选现有内容。
        ::SendMessageW(native_edit, EM_SETSEL, 0, -1);
    }
}

LRESULT ItemEditWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (uMsg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE) {
        // 激活序列尚未结束（随后还有系统 WM_SETFOCUS 抢回焦点），
        // 延迟到自定义消息里再聚焦原生 EDIT。
        ::PostMessage(m_hWnd, kFocusEditMsg, 0, 0);
    }
    if (uMsg == kFocusEditMsg) {
        // 原生 EDIT 子窗口失焦即自毁（CEditWndWin32::OnKillFocus → WM_CLOSE），
        // 重新激活时需重建并把 Win32 焦点交给它，否则键盘输入进不来。
        focus_index_ = 0;
        appui::FocusNativeEdit(m_pm, name_input_, m_hWnd);
        bHandled = TRUE;
        return 0;
    }
    if (uMsg == appui::kCycleFocusMsg) {
        // 子类化 EDIT 内按 Tab 转发来的焦点轮换请求。
        CycleInputFocus();
        bHandled = TRUE;
        return 0;
    }
    if (uMsg == WM_CHAR && wParam != VK_RETURN && wParam != VK_ESCAPE) {
        // 字符到达顶层窗口 = 原生 EDIT 没拿到 Win32 焦点（正常时字符直接进 EDIT）。
        // 尽力转交，避免键盘输入丢失。
        HWND native_edit = appui::FocusNativeEdit(m_pm, name_input_, m_hWnd);
        if (native_edit != nullptr) {
            ::SendMessage(native_edit, WM_CHAR, wParam, lParam);
            bHandled = TRUE;
            return 0;
        }
    }
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_TAB) {
            // 焦点不在原生 EDIT 内时 Tab 直接到达顶层窗口。
            CycleInputFocus();
            bHandled = TRUE;
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            if (on_done_) {
                on_done_(false, item_id_, {}, {}, {}, {});
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
        // 任意空白区域拖拽窗口（输入框/按钮/图标除外）——对齐 VB6 无边框窗体行为。
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

void ItemEditWindow::Notify(TNotifyUI& msg) {
    if (_tcscmp(msg.sType, DUI_MSGTYPE_RETURN) == 0) {
        // 原生 EDIT 内按 Enter 时由 CEditWndWin32 转发 RETURN 通知。
        Confirm();
        return;
    }
    if (_tcscmp(msg.sType, DUI_MSGTYPE_CLICK) == 0 && msg.pSender != nullptr) {
        const CDuiString name = msg.pSender->GetName();
        if (name == _T("item_dialog_ok")) {
            Confirm();
            return;
        }
        if (name == _T("item_dialog_cancel")) {
            if (on_done_) {
                on_done_(false, item_id_, {}, {}, {}, {});
            }
            Close();
            return;
        }
        if (name == _T("item_dialog_browse")) {
            const std::wstring file = core::PickOpenPath(m_hWnd, L"可执行文件 (*.exe)\0*.exe\0所有文件 (*.*)\0*.*\0");
            if (!file.empty()) {
                target_input_->SetText(file.c_str());
                RefreshIcon();
            }
            return;
        }
        if (name == _T("item_dialog_icon_default")) {
            icon_location_ = launcher::util::WideToUtf8(target_input_->GetText().GetData());
            RefreshIcon();
            return;
        }
        if (name == _T("item_dialog_icon_change")) {
            const std::wstring file = core::PickOpenPath(m_hWnd, L"图标来源 (*.ico;*.exe;*.dll)\0*.ico;*.exe;*.dll\0所有文件 (*.*)\0*.*\0");
            if (!file.empty()) {
                icon_location_ = launcher::util::WideToUtf8(file);
                RefreshIcon();
            }
            return;
        }
    }

    if (_tcscmp(msg.sType, DUI_MSGTYPE_TEXTCHANGED) == 0 && msg.pSender != nullptr &&
        msg.pSender->GetName() == _T("item_dialog_target_input")) {
        RefreshIcon();
    }
}

void ItemEditWindow::OnFinalMessage(HWND hWnd) {
    WindowImplBase::OnFinalMessage(hWnd);
    delete this;
}
