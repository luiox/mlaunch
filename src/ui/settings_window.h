#pragma once

#include <UIlib.h>

#include <functional>
#include <string>

#include "launcher_core.h"
#include "ui_controls.h"

/**
 * @brief Borderless popup window editing launcher settings (MVP).
 *
 * 对齐 ItemEditWindow 的弹出窗模式：Esc 取消、Enter 确认、任意空白拖拽。
 * 字段：热键、执行后最小化、默认窗口宽高、分组栏宽度。
 */
class SettingsWindow : public DuiLib::WindowImplBase {
public:
    using DoneCallback = std::function<void(bool confirmed, const core::Settings& settings)>;

    SettingsWindow(const core::Settings& initial, DoneCallback on_done);

    LPCTSTR GetWindowClassName() const override { return _T("MLaunchSettingsWindow"); }
    DuiLib::CDuiString GetSkinFile() override { return _T(""); }
    void OnFinalMessage(HWND hWnd) override;
    void Notify(DuiLib::TNotifyUI& msg) override;
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;

    /** @brief Create centered on owner window and show. */
    void CreateAndShow(HWND owner_hwnd);

protected:
    void Confirm();
    bool PointOnEditableControl(POINT pt) const;
    /** @brief Tab 轮换：热键 → 宽 → 高 → 分组栏宽度。 */
    void CycleInputFocus();
    /** @brief 热键捕获：点击输入框进入，录制下一个组合键。 */
    void StartHotkeyCapture();
    void CancelHotkeyCapture();

private:
    DoneCallback on_done_;
    core::Settings draft_;
    int focus_index_ = 0;

    DuiLib::CEditUI* hotkey_input_ = nullptr;
    appui::CheckBoxUI* hide_check_ = nullptr;
    DuiLib::CEditUI* width_input_ = nullptr;
    DuiLib::CEditUI* height_input_ = nullptr;
    DuiLib::CEditUI* panel_input_ = nullptr;

    /** @brief 热键捕获进行中：键盘事件被拦截用于录制组合键。 */
    bool capturing_ = false;
    DuiLib::CDuiString capture_prev_;
};
