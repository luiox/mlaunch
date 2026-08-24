#pragma once

#include <UIlib.h>

#include <string>
#include <vector>

class AppWindow;

class ListController {
public:
    explicit ListController(AppWindow& owner);

    void RenderGroups();
    void RenderItems();
    void SelectGroupByIndex(int index);
    bool SelectListRowFromPoint(DuiLib::CListUI* list, const std::vector<std::string>& ids, const POINT& client_point, std::string* selected_id);

private:
    AppWindow& owner_;
};
