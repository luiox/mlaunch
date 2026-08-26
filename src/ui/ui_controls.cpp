#include "ui_controls.h"

using namespace DuiLib;

namespace appui {

const Theme& GetTheme() {
    static const Theme theme{};
    return theme;
}

void ButtonUI::SetStateColors(DWORD normal, DWORD hot, DWORD pushed) {
    normal_color_ = normal;
    hot_color_ = hot;
    pushed_color_ = pushed;
}

void ButtonUI::SetActive(bool active) {
    if (active_ == active) {
        return;
    }
    active_ = active;
    Invalidate();
}

void ButtonUI::PaintStatusImage(DuiLib::UIRender* pRender) {
    DWORD color = normal_color_;
    if (active_ && pushed_color_ != 0) {
        color = pushed_color_;
    } else if (IsPushedState() && pushed_color_ != 0) {
        color = pushed_color_;
    } else if (IsHotState() && hot_color_ != 0) {
        color = hot_color_;
    }
    if (color != 0) {
        pRender->DrawColor(m_rcItem, DuiLib::CDuiSize(0, 0), color);
    }
    CButtonUI::PaintStatusImage(pRender);
}

CButtonUI* MakeTextButton(LPCTSTR name, LPCTSTR text, int width) {
    auto* button = new ButtonUI();
    button->SetName(name);
    button->SetText(text);
    if (width > 0) {
        button->SetFixedWidth(width);
    }
    button->SetFixedHeight(28);
    // 与主窗统一体系：常态 E6E6E6，悬停/按下 D5D5D5，无边框。
    button->SetStateColors(0xFFE6E6E6, 0xFFD5D5D5, 0xFFD5D5D5);
    button->SetTextColor(0xFF1A1A1A);
    button->SetAttribute(_T("bordercolor"), _T("0x00000000"));
    button->SetAttribute(_T("bordersize"), _T("0"));
    return button;
}

CheckBoxUI::CheckBoxUI() {
    SetFixedHeight(26);
    SetTextColor(0xFF1A1A1A);
    SetStateColors(0x00000000, 0x00000000, 0x00000000);
    SetAttribute(_T("bordercolor"), _T("0x00000000"));
    SetAttribute(_T("bordersize"), _T("0"));
    // 文本让位勾选盒（盒 14px + 8px 间距）。
    SetAttribute(_T("textpadding"), _T("22,0,4,0"));
}

void CheckBoxUI::SetChecked(bool checked) {
    if (checked_ == checked) {
        return;
    }
    checked_ = checked;
    Invalidate();
}

bool CheckBoxUI::Activate() {
    if (!ButtonUI::Activate()) {
        return false;
    }
    checked_ = !checked_;
    Invalidate();
    return true;
}

void CheckBoxUI::PaintStatusImage(DuiLib::UIRender* pRender) {
    ButtonUI::PaintStatusImage(pRender);

    constexpr int kBox = 14;
    const int cy = (m_rcItem.top + m_rcItem.bottom) / 2;
    const DuiLib::CDuiRect box{m_rcItem.left, cy - kBox / 2, m_rcItem.left + kBox, cy + kBox / 2};
    const bool hot = IsHotState() || IsPushedState();
    pRender->DrawColor(box, DuiLib::CDuiSize(0, 0), checked_ ? 0xFF1A73E8 : 0xFFFFFFFF);
    pRender->DrawRect(box, 1, checked_ ? 0xFF1A73E8 : (hot ? 0xFF8A8A8A : 0xFFB8B8B8));
    if (checked_) {
        // 白色对勾（两段线，短撇 + 长捺）。
        pRender->DrawLine(box.left + 3, cy + 1, box.left + 6, box.bottom - 4, 2, 0xFFFFFFFF);
        pRender->DrawLine(box.left + 6, box.bottom - 4, box.left + 11, box.top + 3, 2, 0xFFFFFFFF);
    }
}

IconButtonUI::IconButtonUI() {
    const Theme& theme = GetTheme();
    SetText(_T(""));
    // 三个顶栏按钮统一等大正方形，常态透明，悬停才显示灰底。
    SetFixedWidth(30);
    SetFixedHeight(30);
    SetTextColor(theme.text);
    SetStateColors(0x00000000, 0xFFD0D0D0, 0xFFC4C4C4);
    SetAttribute(_T("bordercolor"), theme.border_color);
    SetAttribute(_T("bordersize"), theme.border_zero);
}

void IconButtonUI::SetSvgImage(const CDuiString& image_attr) {
    SetAttribute(_T("normalimage"), image_attr.GetData());
    SetAttribute(_T("hotimage"), image_attr.GetData());
    SetAttribute(_T("pushedimage"), image_attr.GetData());
}

GroupListUI::GroupListUI() {
    const Theme& theme = GetTheme();
    // 对齐参考图采样：分组区 E6E6E6 灰底，选中条 D2D2D2（与顶栏同灰阶体系）。
    SetBkColor(theme.panel);
    auto* list_info = GetListInfo();
    list_info->SetItemBkColor(0x00000000);
    list_info->SetSelectedItemBkColor(0xFFD2D2D2);
    list_info->SetHotItemBkColor(0xFFDCDCDC);
    SetAttribute(_T("bordercolor"), theme.list_border_color);
    SetAttribute(_T("bordersize"), theme.border_zero);
    // 参考图选中条右缘与条目区之间留 1px 背景缝隙（条带宽 = 面板宽 - 1）。
    SetAttribute(_T("inset"), _T("0,0,1,0"));
    // CListUI 默认带 Header，占位会造成顶部“空白条”，此处强制隐藏。
    if (GetHeader() != nullptr) {
        GetHeader()->SetVisible(false);
    }
    SetChildPadding(0);
}

ItemListUI::ItemListUI() {
    const Theme& theme = GetTheme();
    SetBkColor(theme.white);
    auto* list_info = GetListInfo();
    list_info->SetItemBkColor(0x00000000);
    list_info->SetSelectedItemBkColor(0xFFE0E0E0);
    list_info->SetHotItemBkColor(0xFFF5F5F5);
    SetAttribute(_T("bordercolor"), theme.border_color);
    SetAttribute(_T("bordersize"), theme.border_zero);
    SetAttribute(_T("inset"), theme.inset_zero);
    // 搜索结果与启动项共用该列表，同样去掉默认 Header 占位。
    if (GetHeader() != nullptr) {
        GetHeader()->SetVisible(false);
    }
    SetChildPadding(0);
}

SearchBoxUI::SearchBoxUI() {
    const Theme& theme = GetTheme();
    SetFixedHeight(30);
    SetFont(1);
    SetTextColor(theme.text);
    // 对齐 VB6 原版：Search.UserControl 底色 D2D2D2（由 search_bar 容器承担），
    // Text1 为默认白底无边框 TextBox——输入区自身白底。
    // 不设置 nativebkcolor——该路径的画刷会让原生 EDIT 背景变黑，且原文本框本就是白底。
    SetBkColor(0xFFFFFFFF);
    SetAttribute(_T("bordercolor"), _T("0xFFFFFFFF"));
    SetAttribute(_T("bordersize"), _T("0"));
    SetAttribute(_T("textpadding"), _T("8,4,8,4"));
}

TitleBarUI::TitleBarUI() {
    const Theme& theme = GetTheme();
    SetName(_T("top_bar"));
    SetFixedHeight(35);
    SetBkColor(theme.panel);
    // 按钮在顶栏内垂直居中，标题拉伸占满整行。
    SetAttribute(_T("childvalign"), _T("center"));
    SetAttribute(_T("childpadding"), _T("0"));
    SetAttribute(_T("inset"), theme.title_inset);
    SetAttribute(_T("bordercolor"), theme.border_color);
    SetAttribute(_T("bordersize"), theme.title_border_size);
}

bool GroupRowUI::DoPaint(DuiLib::UIRender* pRender, const DuiLib::CDuiRect& rcPaint, DuiLib::CControlUI* pStopControl) {
    const bool handled = CListContainerElementUI::DoPaint(pRender, rcPaint, pStopControl);
    if (IsSelected()) {
        pRender->DrawRect(m_rcItem, 1, 0xFFCDCDCD);
    }
    return handled;
}

void ApplyFlatScrollbar(DuiLib::CListUI* list, const DuiLib::CDuiString& thumb_attr, DWORD track_bkcolor) {
    if (list == nullptr) {
        return;
    }
    list->EnableScrollBar(true, false);
    auto* vsb = list->GetVerticalScrollBar();
    if (vsb == nullptr) {
        return;
    }
    vsb->SetFixedWidth(12);
    vsb->SetBkColor(track_bkcolor);
    vsb->SetShowButton1(false);
    vsb->SetShowButton2(false);
    if (!thumb_attr.IsEmpty()) {
        vsb->SetThumbNormalImage(thumb_attr.GetData());
        vsb->SetThumbHotImage(thumb_attr.GetData());
        vsb->SetThumbPushedImage(thumb_attr.GetData());
    }
}

} // namespace appui
