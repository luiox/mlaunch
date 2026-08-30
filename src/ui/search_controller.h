#pragma once

#include <string>

#include "constants.h"

class AppWindow;

class SearchController {
public:
    explicit SearchController(AppWindow& owner);

    bool IsSearchMode() const;
    void UpdateSearchUi();
    void ToggleSearchMode();
    void HandleInputChanged();
    /** @brief 搜索结果列表按行移动选择（delta=+1/-1，跳过提示占位行，不环绕）。 */
    void MoveSearchSelection(int delta);

    int GetActiveCommand() const { return active_command_; }
    const std::string& GetBaiduKeyword() const { return baidu_keyword_; }

    static int ParseCommand(const std::string& input, std::string* out_keyword);
    static std::string CommandIdToItemId(int cmd_id);

private:
    AppWindow& owner_;
    int active_command_ = launcher::constants::search_cmd::kNone;
    std::string baidu_keyword_;
};
