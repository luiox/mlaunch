#pragma once

#include <UIlib.h>

#include <filesystem>
#include <string>

class AppWindow;

/** @brief 菜单与 Shell 集成：备份恢复/主菜单/分组/条目右键菜单、导入导出、
 *  管理员运行、资源管理器菜单、系统预设项与锁定/自动隐藏切换。 */
class MenuController {
public:
    explicit MenuController(AppWindow& owner);

    void ShowBackupRecoveryMenu();
    void ExecuteBackupCommand(UINT command_id);
    void ShowGroupContextMenu(const POINT& screen_point);
    void ShowItemContextMenu(const POINT& screen_point);
    void ShowMainContextMenu(const POINT& screen_point, bool right_align = false);
    void ExecuteMainCommand(UINT command_id);
    void ExecuteGroupCommand(UINT command_id);
    void ExecuteItemCommand(UINT command_id);
    void ExecuteSearchCommand(const std::string& item_id);
    bool DeleteActiveGroup();
    bool MoveSelectedItemToGroup(const std::string& target_group_id);
    bool RunSelectedItemAsAdmin();
    bool OpenSelectedItemFolder();
    bool ShowSelectedItemShellMenu();
    bool CopySelectedItemPath();
    bool ImportPonerFile(const std::filesystem::path& path);
    /** @brief 主菜单"新建项目"的系统条目预设（计算机/控制面板/回收站/注销/关机/重启/空项目）。 */
    void AddPresetSystemItem(UINT command_id);
    /** @brief 切换锁定布局（禁窗口拖动/缩放/拖拽重排），立即生效并持久化。 */
    void ToggleLayoutLock();
    /** @brief 切换失焦自动隐藏（热键唤回），立即生效并持久化。 */
    void ToggleAutoHide();
    /** @brief 全量条目路径在绝对与 %pr%/%cr% 占位符间转换（便携模式）。 */
    void ConvertItemPathsMenu(bool to_relative);
    std::wstring PickJsonFilePath() const;
    std::wstring PickSaveJsonFilePath() const;

private:
    AppWindow& owner_;
};
