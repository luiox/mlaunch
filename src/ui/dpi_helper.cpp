#include "dpi_helper.h"

namespace appui {

namespace {

UINT QueryWindowDpi(HWND hwnd) {
    if (hwnd == nullptr) {
        return 96;
    }
    const UINT dpi = ::GetDpiForWindow(hwnd);
    return (dpi > 0) ? dpi : 96;
}

} // namespace

void AlignPaintManagerDpi(DuiLib::CPaintManagerUI& pm, HWND hwnd) {
    const UINT dpi = QueryWindowDpi(hwnd);
    if (pm.GetDPIObj()->GetDPI() == static_cast<int>(dpi)) {
        return;
    }
    pm.GetDPIObj()->SetScale(dpi);
    // WM_CREATE 阶段字体/图像资产尚未初始化（root 为空），此时只需设好缩放，
    // 之后创建的控件自然按新缩放取值；已有资产时才需要重建刷新。
    if (pm.GetRoot() != nullptr) {
        pm.ResetDPIAssets();
        pm.GetRoot()->NeedUpdate();
    }
}

void ScaleDialogToWindowDpi(DuiLib::CPaintManagerUI& pm, HWND hwnd, HWND owner) {
    const UINT dpi = QueryWindowDpi(hwnd);
    if (pm.GetDPIObj()->GetDPI() == static_cast<int>(dpi)) {
        return;
    }
    // SetDPI 按新旧 scale 比例放大窗口（对话框按逻辑尺寸创建，正好需要这一步），
    // 并重建字体/图像缓存；随后相对 owner 重新居中，避免向右下偏移。
    pm.SetDPI(dpi);
    if (owner == nullptr) {
        return;
    }
    RECT owner_rect{};
    RECT self_rect{};
    if (!::GetWindowRect(owner, &owner_rect) || !::GetWindowRect(hwnd, &self_rect)) {
        return;
    }
    const int self_cx = self_rect.right - self_rect.left;
    const int self_cy = self_rect.bottom - self_rect.top;
    const int owner_cx = owner_rect.right - owner_rect.left;
    const int owner_cy = owner_rect.bottom - owner_rect.top;
    ::SetWindowPos(hwnd, nullptr,
                   owner_rect.left + (owner_cx > self_cx ? (owner_cx - self_cx) / 2 : 0),
                   owner_rect.top + (owner_cy > self_cy ? (owner_cy - self_cy) / 2 : 0),
                   0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

} // namespace appui
