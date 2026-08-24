#pragma once

#include <string>

class AppWindow;

class DialogManager {
public:
    explicit DialogManager(AppWindow& owner);

    void OpenGroupDialog(bool rename_mode, const std::string& group_id);
    void CloseGroupDialog();
    void ConfirmGroupDialog();

private:
    AppWindow& owner_;
};
