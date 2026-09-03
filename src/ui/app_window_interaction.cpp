#include "app_window.h"

#include <Windows.h>
#include <shellapi.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "constants.h"
#include "logger.h"
#include "utils/string_util.h"

using namespace DuiLib;

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

    if (uMsg == WM_GETMINMAXINFO) {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        if (info != nullptr) {
            if (info->ptMinTrackSize.x < 420) {
                info->ptMinTrackSize.x = 420;
            }
            if (info->ptMinTrackSize.y < 280) {
                info->ptMinTrackSize.y = 280;
            }
            bHandled = TRUE;
            return 0;
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
        const int min_group = 80;
        const int min_items = 220;
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
            ShowGroupContextMenu(screen_point);
            bHandled = TRUE;
            return 0;
        }

        const bool over_item = SelectListRowFromPoint(items_list_, item_ids_, client_point, &selected_item_id_);
        if (over_item) {
            const int index = items_list_ != nullptr ? items_list_->GetCurSel() : -1;
            if (index >= 0 && index < static_cast<int>(item_group_ids_.size())) {
                selected_item_group_id_ = item_group_ids_[index];
            }
            ShowItemContextMenu(screen_point);
            bHandled = TRUE;
            return 0;
        }

        // 非列表区域（标题栏/空白处）：打开主菜单。
        ShowMainContextMenu(screen_point);
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
