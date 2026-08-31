#pragma once

#include <UIlib.h>

namespace appui {

/**
 * @brief 让 PaintManager 的缩放与窗口所在显示器的真实 DPI 对齐（不改变窗口几何）。
 *
 * fork 在窗口创建时用 GetMainMonitorDPI()（虚拟屏 (1,1) 所在显示器）初始化缩放；
 * 多显示器缩放不一致时会拿到错误值（例如 200% 屏上的窗口拿到 96，整个界面按
 * 逻辑尺寸渲染塞进物理放大后的窗口里，文字图标全部偏小）。本函数在窗口创建
 * 完成后用 GetDpiForWindow 纠正：重设 scale、重建字体、清空图像缓存并重排。
 *
 * 只刷新渲染状态、不动窗口尺寸——调用方需自行用物理像素管理窗口几何
 * （主窗口创建路径已经是物理几何，交给 CPaintManagerUI::SetDPI 会二次放大）。
 */
void AlignPaintManagerDpi(DuiLib::CPaintManagerUI& pm, HWND hwnd);

/**
 * @brief 弹出式对话框专用：同步 scale 并按比例放大窗口，再相对 owner 重新居中。
 *
 * 对话框 Create 时传的是逻辑尺寸（kWindowWidth/kWindowHeight），正好依赖
 * CPaintManagerUI::SetDPI 的“按新旧 scale 比例改窗口大小”行为完成物理放大。
 */
void ScaleDialogToWindowDpi(DuiLib::CPaintManagerUI& pm, HWND hwnd, HWND owner);

} // namespace appui
