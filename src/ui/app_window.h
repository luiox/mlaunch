#pragma once

#include <UIlib.h>

#include <string>
#include <vector>

#include "launcher_core.h"
#include "dialog_manager.h"
#include "icon_manager.h"
#include "list_controller.h"
#include "search_controller.h"
#include "shell_services.h"
#include "status_presenter.h"
#include "ui_builder.h"

class UiBuilder;
class ListController;
class SearchController;
class DialogManager;

class AppWindow : public DuiLib::WindowImplBase {
public:
    /**
     * @brief Construct the main launcher window.
     * @param legacy_root process start directory (captured before DuiLib changes CWD);
     *                    used for legacy Poner Data.json/Poner.cfg discovery.
     */
    explicit AppWindow(std::filesystem::path legacy_root);
    LPCTSTR GetWindowClassName() const override { return _T("NAssistantMainFrame"); }
    DuiLib::CDuiString GetSkinFile() override { return _T(""); }
    DuiLib::CDuiString GetSkinFolder() { return _T(""); }
    bool HasRestoredWindowPlacement() const { return has_restored_window_; }
    bool ShouldStartMaximized() const { return start_maximized_; }
    std::string IconSourceForItem(const core::LaunchItem& item) const;

    void Notify(DuiLib::TNotifyUI& msg) override;
    LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;

protected:
    DuiLib::CControlUI* CreateControl(LPCTSTR pstrClass) override { return nullptr; }

private:
    friend class UiBuilder;
    friend class ListController;
    friend class SearchController;
    friend class DialogManager;

    enum class DragListKind {
        None,
        Groups,
        Items,
    };

    DuiLib::CControlUI* BuildRootUi();
    bool LoadBackendData();
    void RenderGroups();
    void RenderItems();
    void SelectGroupByIndex(int index);
    void LaunchSelectedItem();
    void DeleteSelectedItem();
    void HandleFileDrop(HDROP drop_handle);
    void UpdateSearchUi();
    bool IsSearchMode() const;

    void ShowGroupContextMenu(const POINT& screen_point);
    void ShowItemContextMenu(const POINT& screen_point);
    void ShowMainContextMenu(const POINT& screen_point);
    void ExecuteMainCommand(UINT command_id);
    void ExecuteGroupCommand(UINT command_id);
    void ExecuteItemCommand(UINT command_id);
    void ExecuteSearchCommand(const std::string& item_id);
    void OpenGroupDialog(bool rename_mode, const std::string& group_id);
    void CloseGroupDialog();
    void ConfirmGroupDialog();

    void UndoLastDelete();
    void ShowBackupRecoveryMenu();
    void ExecuteBackupCommand(UINT command_id);
    bool IsActiveGroupRecycleBin() const;

    bool DeleteActiveGroup();
    bool MoveSelectedItemToGroup(const std::string& target_group_id);
    bool RunSelectedItemAsAdmin();
    bool OpenSelectedItemFolder();
    bool ShowSelectedItemShellMenu();
    bool CopySelectedItemPath();
    bool ImportPonerFile(const std::filesystem::path& path);

    void OpenItemDialog(bool edit_mode);
    void CloseItemDialog();
    std::filesystem::path GetExeDir() const;

    std::wstring PickJsonFilePath() const;
    std::string GenerateNewGroupName() const;
    const core::Group* FindActiveGroup() const;
    const core::LaunchItem* FindSelectedItem() const;
    int HitTestListIndex(DuiLib::CListUI* list, const POINT& client_point) const;
    void ResetListDragState();
    bool CommitListDragReorder();
    bool SelectListRowFromPoint(DuiLib::CListUI* list, const std::vector<std::string>& ids, const POINT& client_point, std::string* selected_id);

    static std::string BasenameNoExt(const std::string& path);
    static std::string ToLowerAscii(std::string value);
    bool ContainsCaseInsensitive(const std::string& text, const std::string& keyword) const;
    void RestoreUiState();
    void SaveUiState();
    void MarkUiStateDirty();
    void ScheduleUiStateSave();
    void FlushUiStateIfDirty();

private:
    // Shell 服务必须先于 backend_ 构造，供其注入使用。
    core::ShellLaunchExecutor launch_executor_;
    core::ShellShortcutResolver shortcut_resolver_;

    core::LauncherBackend backend_;
    IconManager icon_manager_;
    StatusPresenter status_;
    UiBuilder ui_builder_;
    ListController list_controller_;
    SearchController search_controller_;
    DialogManager dialog_manager_;

    DuiLib::CListUI* groups_list_ = nullptr;
    DuiLib::CListUI* items_list_ = nullptr;
    DuiLib::CLabelUI* status_line_ = nullptr;
    DuiLib::CControlUI* search_bar_ = nullptr;
    DuiLib::CVerticalLayoutUI* group_panel_ = nullptr;
    DuiLib::CControlUI* panel_splitter_ = nullptr;
    DuiLib::CEditUI* search_input_ = nullptr;
    DuiLib::CVerticalLayoutUI* group_dialog_ = nullptr;
    DuiLib::CLabelUI* group_dialog_title_ = nullptr;
    DuiLib::CEditUI* group_dialog_input_ = nullptr;

    std::vector<std::string> group_ids_;
    std::vector<std::string> item_ids_;
    std::vector<std::string> item_group_ids_;
    std::string active_group_id_;
    std::string selected_item_id_;
    std::string selected_item_group_id_;

    bool search_mode_ = false;
    bool group_dialog_rename_mode_ = false;
    std::string group_dialog_group_id_;

    bool splitter_dragging_ = false;
    int splitter_drag_start_x_ = 0;
    int splitter_start_width_ = 220;
    int splitter_pending_width_ = -1;
    DWORD splitter_last_update_tick_ = 0;

    DragListKind drag_list_kind_ = DragListKind::None;
    bool list_drag_prepared_ = false;
    bool list_dragging_ = false;
    bool list_drag_polling_ = false;
    POINT list_drag_down_point_{0, 0};
    int list_drag_from_index_ = -1;
    int list_drag_hover_index_ = -1;

    bool has_restored_window_ = false;
    bool start_maximized_ = false;

    bool ui_state_dirty_ = false;
    bool ui_state_timer_active_ = false;
};
