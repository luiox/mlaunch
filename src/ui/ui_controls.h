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

// 此 fork 的 CButtonUI 状态色只能走图片（normalbkcolor 等属性不存在，
// 会被静默忽略导致按钮无底色），这里在 PaintStatusImage 里按状态自绘纯色。
class ButtonUI : public DuiLib::CButtonUI {
public:
    LPCTSTR GetClass() const override { return _T("AppButton"); }
    void SetStateColors(DWORD normal, DWORD hot, DWORD pushed);
    // 粘滞按下态（如搜索模式下的放大镜）：激活期间恒显示 pushed 色，直到再次关闭。
    void SetActive(bool active);

protected:
    void PaintStatusImage(DuiLib::UIRender* pRender) override;

private:
    DWORD normal_color_ = 0x00000000;
    DWORD hot_color_ = 0x00000000;
    DWORD pushed_color_ = 0x00000000;
    bool active_ = false;
};

// 主体系文本按钮：E6E6E6 常态 / D5D5D5 悬停按下，无边框，高 28。
DuiLib::CButtonUI* MakeTextButton(LPCTSTR name, LPCTSTR text, int width = 0);

// 自绘复选框：此 fork 的 CCheckBoxUI 勾选态依赖图片资源，无资源时不可见。
// 14px 边框盒 + 勾选蓝底白对勾；点击自切换并发 CLICK 通知（宿主读 IsChecked）。
class CheckBoxUI : public ButtonUI {
public:
    CheckBoxUI();
    LPCTSTR GetClass() const override { return _T("AppCheckBox"); }
    void SetChecked(bool checked);
    bool IsChecked() const { return checked_; }

protected:
    bool Activate() override;
    void PaintStatusImage(DuiLib::UIRender* pRender) override;

private:
    bool checked_ = false;
};

class IconButtonUI : public ButtonUI {
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

// 分组行：参考图选中条为 D2D2D2 填充 + 四周 1px CDCDCD 描边，
// 列表级 item 背景只能填色，描边在行自身 DoPaint 末尾补画（盖过子控件）。
class GroupRowUI : public DuiLib::CListContainerElementUI {
public:
    LPCTSTR GetClass() const override { return _T("AppGroupRow"); }

protected:
    bool DoPaint(DuiLib::UIRender* pRender, const DuiLib::CDuiRect& rcPaint, DuiLib::CControlUI* pStopControl) override;
};

// 启用列表竖向滚动条并配置为参考图的极简样式：
// 12px 宽、无箭头按钮、轨道纯色、滑块用带 1px 描边的九宫格图。
void ApplyFlatScrollbar(DuiLib::CListUI* list, const DuiLib::CDuiString& thumb_attr, DWORD track_bkcolor);

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
