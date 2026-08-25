#include "ui_builder.h"

#include "app_window.h"
#include "file_icon_control.h"
#include "icon_manager.h"
#include "ui_controls.h"

using namespace DuiLib;

UiBuilder::UiBuilder(AppWindow& owner)
    : owner_(owner) {}

CControlUI* UiBuilder::BuildRootUi() const {
    auto* root = new CVerticalLayoutUI();
    root->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    root->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    root->SetAttribute(_T("bordersize"), _T("1"));
    root->SetAttribute(_T("inset"), _T("0,0,0,0"));
    root->SetAttribute(_T("childpadding"), _T("0"));

    auto* topBar = new appui::TitleBarUI();

    auto* title = new CLabelUI();
    title->SetText(_T("Poner"));
    title->SetTextColor(0xFF1A1A1A);
    title->SetFont(0);
    title->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    topBar->Add(title);

    auto* fill = new CControlUI();
    topBar->Add(fill);

    auto* searchBtn = new appui::IconButtonUI();
    searchBtn->SetName(_T("searchbtn"));
    const auto search_icon = owner_.icon_manager_.ResolveTopBarIcon(icon::Icon::Search, icon::Icon::Search);
    const CDuiString search_img_n = owner_.icon_manager_.MakeSvgImageAttr(search_icon);
    searchBtn->SetSvgImage(search_img_n);
    topBar->Add(searchBtn);

    auto* menuBtn = new appui::IconButtonUI();
    menuBtn->SetName(_T("menubtn"));
    const auto menu_icon = owner_.icon_manager_.ResolveTopBarIcon(icon::Icon::Menu, icon::Icon::Menu);
    const CDuiString menu_img_n = owner_.icon_manager_.MakeSvgImageAttr(menu_icon);
    menuBtn->SetSvgImage(menu_img_n);
    topBar->Add(menuBtn);

    auto* closeBtn = new appui::IconButtonUI();
    closeBtn->SetName(_T("closebtn"));
    const auto close_icon = owner_.icon_manager_.ResolveTopBarIcon(icon::Icon::Close, icon::Icon::Clear);
    const CDuiString exit_img_n = owner_.icon_manager_.MakeSvgImageAttr(close_icon);
    closeBtn->SetSvgImage(exit_img_n);
    topBar->Add(closeBtn);

    root->Add(topBar);

    auto* searchBar = new CHorizontalLayoutUI();
    searchBar->SetName(_T("search_bar"));
    searchBar->SetVisible(false);
    searchBar->SetFixedHeight(0);
    searchBar->SetAttribute(_T("inset"), _T("0,0,0,0"));
    searchBar->SetAttribute(_T("childpadding"), _T("0"));
    searchBar->SetAttribute(_T("bkcolor"), _T("0xFFD2D2D2"));

    auto* searchInput = new appui::SearchBoxUI();
    searchInput->SetName(_T("search_input"));
    searchInput->SetVisible(false);
    searchBar->Add(searchInput);

    root->Add(searchBar);

    auto* body = new CHorizontalLayoutUI();
    body->SetName(_T("body_layout"));
    body->SetAttribute(_T("inset"), _T("0,0,0,0"));
    body->SetAttribute(_T("childpadding"), _T("0"));

    auto* groupPanel = new CVerticalLayoutUI();
    groupPanel->SetName(_T("group_panel"));
    groupPanel->SetFixedWidth(220);
    groupPanel->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    groupPanel->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    groupPanel->SetAttribute(_T("bordersize"), _T("0"));
    groupPanel->SetAttribute(_T("inset"), _T("0,0,0,0"));
    groupPanel->SetAttribute(_T("childpadding"), _T("0"));

    auto* groups = new appui::GroupListUI();
    groups->SetName(_T("groups_list"));
    groupPanel->Add(groups);

    body->Add(groupPanel);

    auto* splitter = new CControlUI();
    splitter->SetName(_T("panel_splitter"));
    splitter->SetFixedWidth(1);
    splitter->SetAttribute(_T("bkcolor"), _T("0xFFD2D2D2"));
    body->Add(splitter);

    auto* itemPanel = new CVerticalLayoutUI();
    itemPanel->SetName(_T("item_panel"));
    itemPanel->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    itemPanel->SetAttribute(_T("bordercolor"), _T("0xFFB8C3CF"));
    itemPanel->SetAttribute(_T("bordersize"), _T("0"));
    itemPanel->SetAttribute(_T("inset"), _T("0,0,0,0"));
    itemPanel->SetAttribute(_T("childpadding"), _T("0"));

    auto* items = new appui::ItemListUI();
    items->SetName(_T("items_list"));
    itemPanel->Add(items);

    body->Add(itemPanel);
    root->Add(body);

    // 状态反馈不再占用常驻状态栏（启动器空间宝贵），改为浮动 Toast：
    // 平时隐藏，有消息时显示，由 StatusPresenter 通过定时器自动隐藏。
    auto* status = new CLabelUI();
    status->SetName(_T("status_line"));
    status->SetVisible(false);
    status->SetFloat(true);
    status->SetFloatAlign(DT_CENTER | DT_BOTTOM);
    status->SetFixedWidth(460);
    status->SetFixedHeight(30);
    status->SetAttribute(_T("bkcolor"), _T("0xFF333333"));
    status->SetTextColor(0xFFFFFFFF);
    status->SetAttribute(_T("inset"), _T("10,4,10,4"));
    status->SetTextStyle(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    root->Add(status);

    auto* groupDialog = new CVerticalLayoutUI();
    groupDialog->SetName(_T("group_dialog"));
    groupDialog->SetVisible(false);
    groupDialog->SetFloat(true);
    groupDialog->SetFloatAlign(DT_CENTER | DT_VCENTER);
    groupDialog->SetFixedWidth(360);
    groupDialog->SetFixedHeight(150);
    groupDialog->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    groupDialog->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    groupDialog->SetAttribute(_T("bordersize"), _T("1"));
    groupDialog->SetAttribute(_T("inset"), _T("12,10,12,10"));
    groupDialog->SetAttribute(_T("childpadding"), _T("8"));

    auto* groupDialogTitle = new CLabelUI();
    groupDialogTitle->SetName(_T("group_dialog_title"));
    groupDialogTitle->SetText(_T("新建分组"));
    groupDialogTitle->SetFixedHeight(24);
    groupDialogTitle->SetTextColor(0xFF1A1A1A);
    groupDialogTitle->SetFont(0);
    groupDialog->Add(groupDialogTitle);

    auto* groupDialogInput = new CEditUI();
    groupDialogInput->SetName(_T("group_dialog_input"));
    groupDialogInput->SetFixedHeight(28);
    groupDialogInput->SetAttribute(_T("bordercolor"), _T("0xFFD2D2D2"));
    groupDialogInput->SetAttribute(_T("bkcolor"), _T("0xFFFFFFFF"));
    groupDialogInput->SetAttribute(_T("textpadding"), _T("6,3,6,3"));
    groupDialog->Add(groupDialogInput);

    auto* actions = new CHorizontalLayoutUI();
    actions->SetAttribute(_T("childpadding"), _T("8"));
    actions->SetAttribute(_T("childalign"), _T("right"));

    auto* okButton = new CButtonUI();
    okButton->SetName(_T("group_dialog_ok"));
    okButton->SetText(_T("确定"));
    okButton->SetFixedWidth(88);
    okButton->SetFixedHeight(28);
    okButton->SetAttribute(_T("normalbkcolor"), _T("0xFFE6E6E6"));
    okButton->SetAttribute(_T("hotbkcolor"), _T("0xFFD5D5D5"));
    okButton->SetAttribute(_T("pushedbkcolor"), _T("0xFFD5D5D5"));
    okButton->SetAttribute(_T("textcolor"), _T("0xFF1A1A1A"));
    okButton->SetAttribute(_T("bordercolor"), _T("0x00000000"));
    actions->Add(okButton);

    auto* cancelButton = new CButtonUI();
    cancelButton->SetName(_T("group_dialog_cancel"));
    cancelButton->SetText(_T("取消"));
    cancelButton->SetFixedWidth(88);
    cancelButton->SetFixedHeight(28);
    cancelButton->SetAttribute(_T("normalbkcolor"), _T("0xFFE6E6E6"));
    cancelButton->SetAttribute(_T("hotbkcolor"), _T("0xFFD5D5D5"));
    cancelButton->SetAttribute(_T("pushedbkcolor"), _T("0xFFD5D5D5"));
    cancelButton->SetAttribute(_T("textcolor"), _T("0xFF1A1A1A"));
    cancelButton->SetAttribute(_T("bordercolor"), _T("0x00000000"));
    actions->Add(cancelButton);

    groupDialog->Add(actions);
    root->Add(groupDialog);

    return root;
}
