#include "ui_controls.h"

using namespace DuiLib;

namespace appui {

const Theme& GetTheme() {
    static const Theme theme{};
    return theme;
}

IconButtonUI::IconButtonUI() {
    const Theme& theme = GetTheme();
    SetText(_T(""));
    // 三个顶栏按钮统一等大正方形，常态透明，悬停才显示灰底。
    SetFixedWidth(30);
    SetFixedHeight(30);
    SetTextColor(theme.text);
    SetAttribute(_T("normalbkcolor"), theme.icon_btn_normal);
    SetAttribute(_T("hotbkcolor"), theme.icon_btn_hot);
    SetAttribute(_T("pushedbkcolor"), theme.icon_btn_pushed);
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
    SetBkColor(0xFFD2D2D2);
    SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    SetAttribute(_T("bordersize"), _T("0"));
    SetAttribute(_T("nativebkcolor"), _T("0xFFD2D2D2"));
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
