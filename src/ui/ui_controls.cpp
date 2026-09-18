#include "ui_controls.h"

using namespace DuiLib;

namespace appui {

SearchBoxUI::SearchBoxUI() {
    SetFont(1);
}

TitleBarUI::TitleBarUI() {
    SetName(_T("top_bar"));
}

void ApplyFlatScrollbar(DuiLib::CListUI* list, const DuiLib::CDuiString& thumb_attr,
                        DWORD track_bkcolor) {
    uikit::duilib::ApplyFlatScrollbar(list, track_bkcolor, thumb_attr);
}

} // namespace appui
