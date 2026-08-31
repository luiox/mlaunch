#include "shadow_window.h"

namespace {

constexpr wchar_t kClassName[] = L"MLaunchShadowWndClass";
constexpr wchar_t kTitle[] = L"MLaunchShadow";
// 对齐 VB6 frmShadow：硬边剪影，右下偏移，整体半透明黑。
constexpr int kOffsetPx = 7;
constexpr BYTE kShadowAlpha = 60;

} // namespace

ShadowWindow::~ShadowWindow() {
    Detach();
}

void ShadowWindow::Attach(HWND main_hwnd) {
    if (main_hwnd == nullptr || hwnd_ != nullptr) {
        return;
    }
    main_ = main_hwnd;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    if (!::GetClassInfoExW(::GetModuleHandleW(nullptr), kClassName, &wc)) {
        wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &ShadowWindow::WndProc;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.hbrBackground = ::CreateSolidBrush(RGB(0, 0, 0));
        wc.lpszClassName = kClassName;
        ::RegisterClassExW(&wc);
    }

    hwnd_ = ::CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kClassName, kTitle, WS_POPUP,
        0, 0, 1, 1, nullptr, nullptr, ::GetModuleHandleW(nullptr), nullptr);
    if (hwnd_ == nullptr) {
        main_ = nullptr;
        return;
    }
    ::SetLayeredWindowAttributes(hwnd_, 0, kShadowAlpha, LWA_ALPHA);
    Sync();
}

void ShadowWindow::Detach() {
    if (hwnd_ != nullptr) {
        ::DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    main_ = nullptr;
}

void ShadowWindow::Sync() {
    if (main_ == nullptr || hwnd_ == nullptr || !::IsWindow(hwnd_)) {
        return;
    }

    // 最小化/最大化/隐藏时不显示偏移剪影（最大化贴边会露出难看黑边）。
    if (!::IsWindowVisible(main_) || ::IsIconic(main_) || ::IsZoomed(main_)) {
        if (::IsWindowVisible(hwnd_)) {
            ::ShowWindow(hwnd_, SW_HIDE);
        }
        return;
    }

    RECT rc{};
    if (!::GetWindowRect(main_, &rc)) {
        return;
    }

    // 插到主窗 Z 序正下方，随主窗显隐；SWP_NOACTIVATE 避免抢焦点。
    // 偏移是 96 基准逻辑值，随主窗 DPI 换算，高缩放下剪影不会贴得太近。
    const UINT dpi = ::GetDpiForWindow(main_);
    const int offset = ::MulDiv(kOffsetPx, (dpi > 0 ? dpi : 96), 96);
    ::SetWindowPos(hwnd_, main_,
                   rc.left + offset, rc.top + offset,
                   rc.right - rc.left, rc.bottom - rc.top,
                   SWP_NOACTIVATE);
    if (!::IsWindowVisible(hwnd_)) {
        ::ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    }
}

LRESULT CALLBACK ShadowWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        ::BeginPaint(hwnd, &ps);
        ::EndPaint(hwnd, &ps);
        return 0;
    }
    // 点击穿透 + 不响应键鼠，其余全部交默认处理。
    default:
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
