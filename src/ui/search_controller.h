#pragma once

#include <string>

#include "constants.h"
#include "tool_registry.h"

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

    // —— 工具插件（src/tools 注册表）——
    /** @brief 命中的工具关键字；空串表示本次输入不是工具调用。 */
    const std::string& GetToolKeyword() const { return tool_keyword_; }
    const std::string& GetToolArgs() const { return tool_args_; }
    /** @brief HandleInputChanged 时缓存的工具执行结果（渲染与回车复制共用）。 */
    const tools::ToolOutput& GetToolOutput() const { return tool_output_; }
    /** @brief 清空工具状态（退出搜索模式时调用，防止旧结果泄漏到下一次）。 */
    void ClearToolState();

    static int ParseCommand(const std::string& input, std::string* out_keyword);
    static std::string CommandIdToItemId(int cmd_id);

private:
    AppWindow& owner_;
    int active_command_ = launcher::constants::search_cmd::kNone;
    std::string baidu_keyword_;

    std::string tool_keyword_;
    std::string tool_args_;
    tools::ToolOutput tool_output_;
};
