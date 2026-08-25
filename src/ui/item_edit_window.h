#pragma once

#include <UIlib.h>

#include <functional>
#include <string>

#include "file_icon_control.h"
#include "launcher_core.h"

class AppWindow;

/**
 * @brief Borderless popup window for creating/editing a launch item.
 *
 * 对齐 VB6 frmEdit：独立无边框窗口，可在任意空白区域拖拽（含拖出主窗范围），
 * Esc 取消、Enter 确认。窗口销毁时自删除（OnFinalMessage）。
 */
class ItemEditWindow : public DuiLib::WindowImplBase {
public:
    using DoneCallback = std::function<void(bool confirmed,
                                            const std::string& item_id,
                                            const std::string& name,
                                            const std::string& target_path,
                                            const std::string& arguments,
                                            const std::string& icon_location)>;

    ItemEditWindow(AppWindow& owner,
                   bool edit_mode,
                   const std::string& group_id,
                   const std::string& item_id,
                   const core::LaunchItem* initial,
                   DoneCallback on_done);

    LPCTSTR GetWindowClassName() const override { return _T("MLaunchItemEditWindow"); }
    DuiLib::CDuiString GetSkinFile() override { return _T(""); }
    void OnFinalMessage(HWND hWnd) override;
    void Notify(DuiLib::TNotifyUI& msg) override;
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;

    /** @brief Create centered on owner window, populate controls and show. */
    void CreateAndShow(HWND owner_hwnd);

protected:
    void Confirm();
    void RefreshIcon();
    bool PointOnEditableControl(POINT pt) const;

private:
    AppWindow& owner_;
    DoneCallback on_done_;
    bool edit_mode_ = false;
    std::string item_id_;
    std::string initial_name_;
    std::string initial_target_;
    std::string initial_args_;
    std::string icon_location_;

    DuiLib::CEditUI* name_input_ = nullptr;
    DuiLib::CEditUI* target_input_ = nullptr;
    DuiLib::CEditUI* args_input_ = nullptr;
    FileIconControl* icon_preview_ = nullptr;
};
