#pragma once

#include <UIlib.h>

namespace appui {

/**
 * @brief Ensure the given CEditUI has its native EDIT child created and Win32-focused,
 *        then subclass it so Tab (focus cycling) and Ctrl+A (select all) work.
 *
 * 本 fork 的原生 EDIT（类名 EditWnd）失焦自毁；非 layered 模式下它忽略 Tab 且
 * 键盘事件不冒泡到顶层窗口。子类化后：Tab → 投递 kCycleFocusMsg 给宿主窗口、
 * Ctrl+A → EM_SETSEL 全选。宿主在 HandleCustomMessage 处理 kCycleFocusMsg。
 */
HWND FocusNativeEdit(DuiLib::CPaintManagerUI& pm, DuiLib::CEditUI* input, HWND dialog);

/** @brief Move DuiLib tab focus forward and focus the new control's native EDIT. */
HWND CycleEditFocus(DuiLib::CPaintManagerUI& pm, HWND dialog);

/** @brief Message posted to the dialog when the subclassed EDIT sees VK_TAB. */
constexpr UINT kCycleFocusMsg = WM_APP + 0x1C;

} // namespace appui
