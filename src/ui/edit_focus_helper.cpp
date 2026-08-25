#include "edit_focus_helper.h"

#include <cstring>

using namespace DuiLib;

namespace appui {

namespace {

constexpr wchar_t kOldProcProp[] = L"MLaunchOldEditProc";

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WNDPROC old_proc = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kOldProcProp));
    if (old_proc == nullptr) {
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    if (msg == WM_CHAR && wParam == 0x01) {
        // Ctrl+A 以控制字符形态到达（IME 开启时 WM_KEYDOWN 是 VK_PROCESSKEY）。
        ::SendMessageW(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    if (msg == WM_KEYDOWN) {
        if (wParam == 'A' && (::GetKeyState(VK_CONTROL) & 0x8000) != 0) {
            ::SendMessageW(hwnd, EM_SETSEL, 0, -1);
            return 0;
        }
        if (wParam == VK_TAB) {
            // 焦点在原生 EDIT 内时键不会到顶层窗口，这里代为转发轮换请求。
            HWND dialog = ::GetParent(hwnd);
            if (dialog != nullptr) {
                ::PostMessageW(dialog, kCycleFocusMsg, 0, 0);
            }
            return 0;
        }
    }
    if (msg == WM_NCDESTROY) {
        ::RemovePropW(hwnd, kOldProcProp);
        return ::CallWindowProcW(old_proc, hwnd, msg, wParam, lParam);
    }
    return ::CallWindowProcW(old_proc, hwnd, msg, wParam, lParam);
}

void SubclassNativeEdit(HWND native_edit) {
    if (native_edit == nullptr || ::GetPropW(native_edit, kOldProcProp) != nullptr) {
        return;
    }
    WNDPROC old_proc = reinterpret_cast<WNDPROC>(
        ::SetWindowLongPtrW(native_edit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditSubclassProc)));
    if (old_proc != nullptr) {
        ::SetPropW(native_edit, kOldProcProp, reinterpret_cast<HANDLE>(old_proc));
    }
}

} // namespace

HWND FocusNativeEdit(CPaintManagerUI& pm, CEditUI* input, HWND dialog) {
    if (input == nullptr) {
        return nullptr;
    }

    auto find_edit = [&dialog]() -> HWND {
        HWND native_edit = ::FindWindowExW(dialog, nullptr, L"EditWnd", nullptr);
        if (native_edit == nullptr) {
            native_edit = ::FindWindowExW(dialog, nullptr, L"Edit", nullptr);
        }
        return native_edit;
    };

    if (pm.GetFocus() != input) {
        // 切换目标控件：先同步销毁现存原生 EDIT（WM_CLOSE → DefaultWndProc 自毁，
        // owner 的 m_pWindow 同步清空），否则 FindWindowExW 会命中旧控件的 EDIT，
        // 新控件的 UIEVENT_SETFOCUS 也不会触发创建。
        HWND existing = find_edit();
        if (existing != nullptr) {
            ::SendMessageW(existing, WM_CLOSE, 0, 0);
        }
        pm.SetFocus(input);
    } else if (find_edit() == nullptr) {
        // manager 焦点已在目标控件时 SetFocus 会早退、不发 UIEVENT_SETFOCUS，
        // 原生 EDIT 不会创建；先清空焦点再设回，强制触发创建。
        pm.SetFocus(nullptr);
        pm.SetFocus(input);
    }

    HWND native_edit = find_edit();
    if (native_edit == nullptr) {
        // 仍无则模拟真实点击路径（BUTTONDOWN 分支要求 IsFocused，上一步已满足）。
        TEventUI ev = {0};
        ev.Type = UIEVENT_BUTTONDOWN;
        ev.pSender = input;
        ev.ptMouse = input->GetPos();
        ev.wKeyState = MK_LBUTTON;
        ev.dwTimestamp = ::GetTickCount();
        input->Event(ev);
        native_edit = find_edit();
    }
    if (native_edit != nullptr) {
        ::SetFocus(native_edit);
        SubclassNativeEdit(native_edit);
    }
    return native_edit;
}

HWND CycleEditFocus(CPaintManagerUI& pm, HWND dialog) {
    pm.SetNextTabControl(true);
    CControlUI* focus = pm.GetFocus();
    CEditUI* edit = nullptr;
    if (focus != nullptr) {
        edit = static_cast<CEditUI*>(focus->GetInterface(_T("Edit")));
    }
    return FocusNativeEdit(pm, edit, dialog);
}

} // namespace appui
