#pragma once

#include <UIlib.h>

namespace appui {

struct Theme {
    // VB6 原版使用窗口文字色（近黑），浅灰会显得笔画细。
    DWORD text = 0xFF1A1A1A;
    DWORD panel = 0xFFE6E6E6;
    DWORD white = 0xFFFFFFFF;
    DWORD border = 0xFFD2D2D2;
    DWORD list_border = 0xFFCFD7E0;
    LPCTSTR border_color = _T("0xFFD2D2D2");
    LPCTSTR list_border_color = _T("0xFFCFD7E0");
    // 顶栏图标按钮：常态透明，悬停/按下才出现灰底（对齐 Poner 行为）。
    LPCTSTR icon_btn_normal = _T("0x00000000");
    LPCTSTR icon_btn_hot = _T("0xFFD0D0D0");
    LPCTSTR icon_btn_pushed = _T("0xFFC4C4C4");
    LPCTSTR border_zero = _T("0");
    LPCTSTR border_one = _T("1");
    LPCTSTR inset_zero = _T("0,0,0,0");
    LPCTSTR title_inset = _T("12,0,0,0");
    LPCTSTR title_border_size = _T("0,0,0,0");
    LPCTSTR search_padding = _T("6,2,6,2");
};

const Theme& GetTheme();

class IconButtonUI : public DuiLib::CButtonUI {
public:
    IconButtonUI();
    LPCTSTR GetClass() const override { return _T("AppIconButton"); }
    void SetSvgImage(const DuiLib::CDuiString& image_attr);
};

class GroupListUI : public DuiLib::CListUI {
public:
    GroupListUI();
    LPCTSTR GetClass() const override { return _T("AppGroupList"); }
};

class ItemListUI : public DuiLib::CListUI {
public:
    ItemListUI();
    LPCTSTR GetClass() const override { return _T("AppItemList"); }
};

class SearchBoxUI : public DuiLib::CEditUI {
public:
    SearchBoxUI();
    LPCTSTR GetClass() const override { return _T("AppSearchBox"); }
};

class TitleBarUI : public DuiLib::CHorizontalLayoutUI {
public:
    TitleBarUI();
    LPCTSTR GetClass() const override { return _T("AppTitleBar"); }
};

} // namespace appui
