#pragma once

#include <Windows.h>

/**
 * @brief Hard-edged drop shadow behind the main window (VB6 frmShadow 复刻).
 *
 * 半透明黑色剪影窗：贴在主窗 Z 序正下方，向右下偏移固定像素；
 * 点击穿透（WS_EX_TRANSPARENT）、不抢焦点（WS_EX_NOACTIVATE）、不进 Alt-Tab。
 * 主窗每次 WM_WINDOWPOSCHANGED / WM_SHOWWINDOW 后调用 Sync() 跟随；
 * 主窗最小化/最大化/隐藏时自动隐藏。
 */
class ShadowWindow {
public:
    ~ShadowWindow();
    /** @brief Create the layered shadow window for the given main window. */
    void Attach(HWND main_hwnd);
    /** @brief Destroy the shadow window. */
    void Detach();
    /** @brief Re-position/Show/Hide from the main window's current state. */
    void Sync();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND main_ = nullptr;
    HWND hwnd_ = nullptr;
};
