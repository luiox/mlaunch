#pragma once

#include <filesystem>
#include <string>

class AppWindow;
class ItemEditWindow;

class DialogManager {
public:
    explicit DialogManager(AppWindow& owner);

    void OpenGroupDialog(bool rename_mode, const std::string& group_id);
    void CloseGroupDialog();
    void ConfirmGroupDialog();

    void OpenItemDialog(bool edit_mode, const std::string& group_id, const std::string& item_id);
    void CloseItemDialog();

private:
    void OnItemEditDone(const std::string& group_id, bool confirmed, const std::string& item_id,
                        const std::string& name, const std::string& target,
                        const std::string& args, const std::string& icon_location);

    AppWindow& owner_;
    ItemEditWindow* item_edit_window_ = nullptr;
};
