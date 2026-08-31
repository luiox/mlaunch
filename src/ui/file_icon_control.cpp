#include "file_icon_control.h"

#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>

FileIconControl::~FileIconControl() {
    ResetIcon();
}

void FileIconControl::ResetIcon() {
    if (icon_ != nullptr) {
        DestroyIcon(icon_);
        icon_ = nullptr;
    }
}

HICON FileIconControl::LoadShellIcon(const std::wstring& path, bool prefer_large) {
    SHFILEINFOW file_info{};
    // 高于 100% 缩放时取 32px 源图：列表绘制尺寸会被放大（20 逻辑 → 40 物理），
    // 仍用 16px 小图标的位图会被强行拉伸发虚。
    const DWORD flags = SHGFI_ICON | (prefer_large ? SHGFI_LARGEICON : SHGFI_SMALLICON);

    if (!path.empty()) {
        const DWORD attributes = GetFileAttributesW(path.c_str());
        const DWORD use_flags = (attributes == INVALID_FILE_ATTRIBUTES)
            ? (flags | SHGFI_USEFILEATTRIBUTES)
            : flags;
        if (SHGetFileInfoW(path.c_str(), FILE_ATTRIBUTE_NORMAL, &file_info, sizeof(file_info), use_flags) != 0 && file_info.hIcon != nullptr) {
            return file_info.hIcon;
        }
    }

    if (SHGetFileInfoW(L".exe", FILE_ATTRIBUTE_NORMAL, &file_info, sizeof(file_info), flags | SHGFI_USEFILEATTRIBUTES) != 0) {
        return file_info.hIcon;
    }

    return nullptr;
}

void FileIconControl::SetIconPath(const std::wstring& path) {
    ResetIcon();
    const bool prefer_large = (GetManager() != nullptr && GetManager()->GetDPIObj()->GetScale() > 100);
    icon_ = LoadShellIcon(path, prefer_large);
    NeedUpdate();
}

void FileIconControl::PaintStatusImage(DuiLib::UIRender* pRender) {
    CControlUI::PaintStatusImage(pRender);
    if (pRender == nullptr || icon_ == nullptr) {
        return;
    }

    HDC dc = pRender->GetDC();
    if (dc == nullptr) {
        return;
    }

    RECT rc = GetPos();
    const int width = (std::max)(0, static_cast<int>(rc.right - rc.left));
    const int height = (std::max)(0, static_cast<int>(rc.bottom - rc.top));
    // GetPos 是物理像素；目标 16 逻辑尺寸经缩放后在高 DPI 下保持清晰锐利。
    auto* manager = GetManager();
    const int base_size = (manager != nullptr) ? manager->GetDPIObj()->ScaleInt(16) : 16;
    const int icon_size = (std::max)(1, (std::min)(base_size, (std::min)(width, height) - 2));
    const int draw_x = rc.left + (width - icon_size) / 2;
    const int draw_y = rc.top + (height - icon_size) / 2;
    DrawIconEx(dc, draw_x, draw_y, icon_, icon_size, icon_size, 0, nullptr, DI_NORMAL);
}
