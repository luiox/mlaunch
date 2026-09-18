#pragma once

#include <UIlib.h>

// 控件层已上收至 uikit（视觉预制层）：本头从「手绘控件集」瘦身为「消费别名 +
// mlaunch 专属接线」。主题值一律来自 uikit::ActiveTheme()，这里不再有任何色值。
// 历史：这些控件 2026-09 从 mlaunch src/ui 反向提炼进 luiox/uikit（PR uikit#2），
// 视觉口径（色值/尺寸/圆角）由 uikit design/*.json 的 L1 令牌统一定义。
#include "uikit/duilib/controls.h"

namespace appui {

// 与 uikit 同名控件直接取用：类型等价，绘制期实时读 ActiveTheme()（支持切主题）。
using ButtonUI = uikit::duilib::ButtonUI;
using CheckBoxUI = uikit::duilib::CheckBoxUI;
using IconButtonUI = uikit::duilib::IconButtonUI;
using GroupListUI = uikit::duilib::GroupListUI;
using ItemListUI = uikit::duilib::ItemListUI;
using GroupRowUI = uikit::duilib::GroupRowUI;

using uikit::duilib::MakeTextButton;

// mlaunch 专属接线（应用字体/命名语义，uikit 不越界接管）：
class SearchBoxUI : public uikit::duilib::SearchBoxUI {
public:
    SearchBoxUI();  // 走字体 id 1（微软雅黑 12，app_window/settings/item_edit 注册）
};

class TitleBarUI : public uikit::duilib::TitleBarUI {
public:
    TitleBarUI();  // 保留历史控件名 top_bar
};

// 兼容旧调用序（thumb_attr 在前）；转发 uikit 版，轨道色/宽度语义不变。
void ApplyFlatScrollbar(DuiLib::CListUI* list, const DuiLib::CDuiString& thumb_attr,
                        DWORD track_bkcolor);

} // namespace appui
