#include "dialog_manager.h"

#include <algorithm>
#include <cctype>

#include "app_window.h"
#include "utils/string_util.h"

using namespace DuiLib;

DialogManager::DialogManager(AppWindow& owner)
    : owner_(owner) {}

void DialogManager::OpenGroupDialog(bool rename_mode, const std::string& group_id) {
    if (owner_.group_dialog_ == nullptr || owner_.group_dialog_input_ == nullptr || owner_.group_dialog_title_ == nullptr) {
        owner_.status_.Error("group dialog is not available");
        return;
    }

    owner_.group_dialog_rename_mode_ = rename_mode;
    owner_.group_dialog_group_id_ = group_id;

    if (rename_mode) {
        const core::Group* group = nullptr;
        for (const auto& candidate : owner_.backend_.Data().groups) {
            if (candidate.id == group_id) {
                group = &candidate;
                break;
            }
        }
        if (group == nullptr) {
            owner_.status_.Warn("group not found");
            return;
        }
        owner_.group_dialog_title_->SetText(_T("Rename Group"));
        owner_.group_dialog_input_->SetText(launcher::util::Utf8ToWide(group->name).c_str());
    } else {
        owner_.group_dialog_title_->SetText(_T("Add Group"));
        owner_.group_dialog_input_->SetText(_T(""));
    }

    owner_.group_dialog_->SetVisible(true);
    owner_.group_dialog_input_->SetFocus();
    owner_.m_pm.NeedUpdate();
}

void DialogManager::CloseGroupDialog() {
    if (owner_.group_dialog_ == nullptr) {
        return;
    }
    owner_.group_dialog_->SetVisible(false);
    owner_.group_dialog_group_id_.clear();
    owner_.group_dialog_rename_mode_ = false;
    owner_.m_pm.NeedUpdate();
}

void DialogManager::ConfirmGroupDialog() {
    if (owner_.group_dialog_input_ == nullptr) {
        return;
    }

    const std::string name = launcher::util::WideToUtf8(owner_.group_dialog_input_->GetText().GetData());
    std::string trimmed = name;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
        trimmed.pop_back();
    }

    if (trimmed.empty()) {
        owner_.status_.Warn("group name cannot be empty");
        return;
    }

    std::string error;
    if (owner_.group_dialog_rename_mode_) {
        if (!owner_.backend_.RenameGroup(owner_.group_dialog_group_id_, trimmed, &error)) {
            owner_.status_.Error("rename group failed: " + error);
            return;
        }
        owner_.status_.Info("group renamed");
    } else {
        const std::string created_id = owner_.backend_.AddGroup(trimmed, &error);
        if (created_id.empty()) {
            owner_.status_.Error("add group failed: " + error);
            return;
        }
        owner_.active_group_id_ = created_id;
        owner_.status_.Info("group added");
    }

    CloseGroupDialog();
    owner_.RenderGroups();
    for (int i = 0; i < static_cast<int>(owner_.group_ids_.size()); ++i) {
        if (owner_.group_ids_[i] == owner_.active_group_id_) {
            owner_.SelectGroupByIndex(i);
            break;
        }
    }
}
