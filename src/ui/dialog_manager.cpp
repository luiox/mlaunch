#include "dialog_manager.h"

#include <algorithm>
#include <cctype>

#include "app_window.h"
#include "item_edit_window.h"
#include "logger.h"
#include "settings_window.h"
#include "utils/string_util.h"

using namespace DuiLib;

DialogManager::DialogManager(AppWindow& owner)
    : owner_(owner) {}

void DialogManager::OpenGroupDialog(bool rename_mode, const std::string& group_id) {
    if (owner_.group_dialog_ == nullptr || owner_.group_dialog_input_ == nullptr || owner_.group_dialog_title_ == nullptr) {
        owner_.status_.Error("分组对话框不可用");
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
            owner_.status_.Warn("分组不存在");
            return;
        }
        owner_.group_dialog_title_->SetText(_T("重命名分组"));
        owner_.group_dialog_input_->SetText(launcher::util::Utf8ToWide(group->name).c_str());
    } else {
        owner_.group_dialog_title_->SetText(_T("新建分组"));
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
        owner_.status_.Warn("分组名不能为空");
        return;
    }

    std::string error;
    if (owner_.group_dialog_rename_mode_) {
        if (!owner_.backend_.RenameGroup(owner_.group_dialog_group_id_, trimmed, &error)) {
            owner_.status_.Error("重命名分组失败：" + error);
            return;
        }
        owner_.status_.Info("分组已重命名");
    } else {
        const std::string created_id = owner_.backend_.AddGroup(trimmed, &error);
        if (created_id.empty()) {
            owner_.status_.Error("新建分组失败：" + error);
            return;
        }
        owner_.active_group_id_ = created_id;
        owner_.status_.Info("分组已创建");
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


void DialogManager::OpenItemDialog(bool edit_mode, const std::string& group_id, const std::string& item_id) {
    if (item_edit_window_ != nullptr) {
        // 已打开则置前，避免多份编辑窗。
        ::SetForegroundWindow(*item_edit_window_);
        return;
    }

    core::LaunchItem initial;
    if (edit_mode) {
        const core::LaunchItem* found = nullptr;
        for (const auto& g : owner_.backend_.Data().groups) {
            if (g.id != group_id) continue;
            for (const auto& i : g.items) {
                if (i.id == item_id) { found = &i; break; }
            }
            if (found != nullptr) break;
        }
        if (found == nullptr) {
            owner_.status_.Warn("条目不存在");
            return;
        }
        initial = *found;
    }

    const std::string group_id_copy = group_id;
    const std::string item_id_copy = item_id;
    item_edit_window_ = new ItemEditWindow(
        owner_, edit_mode, group_id_copy, item_id_copy,
        edit_mode ? &initial : nullptr,
        [this, group_id_copy, item_id_copy](bool confirmed, const std::string& item_id,
                                            const std::string& name, const std::string& target,
                                            const std::string& args, const std::string& icon) {
            OnItemEditDone(group_id_copy, confirmed, item_id, name, target, args, icon);
        });
    item_edit_window_->CreateAndShow(owner_.m_hWnd);
}

void DialogManager::CloseItemDialog() {
    if (item_edit_window_ != nullptr) {
        item_edit_window_->Close();
        // Close 触发销毁链，OnFinalMessage 中自删除；此处不触碰指针内容。
        item_edit_window_ = nullptr;
    }
}

void DialogManager::OpenSettingsDialog() {
    if (settings_window_ != nullptr) {
        ::SetForegroundWindow(*settings_window_);
        return;
    }

    settings_window_ = new SettingsWindow(
        owner_.backend_.CurrentSettings(),
        [this](bool confirmed, const core::Settings& settings) {
            OnSettingsDone(confirmed, settings);
        });
    settings_window_->CreateAndShow(owner_.m_hWnd);
}

void DialogManager::CloseSettingsDialog() {
    if (settings_window_ != nullptr) {
        settings_window_->Close();
        settings_window_ = nullptr;
    }
}

void DialogManager::OnSettingsDone(bool confirmed, const core::Settings& settings) {
    settings_window_ = nullptr;
    if (!confirmed) {
        return;
    }

    std::string error;
    if (!owner_.backend_.UpdateSettings(settings, &error)) {
        owner_.status_.Error("保存设置失败：" + error);
        return;
    }

    owner_.ApplySettings();
    owner_.status_.Info("设置已保存");
}

void DialogManager::OnItemEditDone(const std::string& group_id, bool confirmed, const std::string& item_id,
                                   const std::string& name, const std::string& target,
                                   const std::string& args, const std::string& icon_location) {
    item_edit_window_ = nullptr;
    if (!confirmed) {
        return;
    }

    core::ItemInput input;
    if (!item_id.empty()) {
        input.id = item_id;
    }
    input.name = name;
    input.target_path = target;
    input.arguments = args;
    input.icon_location = icon_location;
    input.item_type = std::string("app");
    input.enabled = true;

    std::string error;
    if (!owner_.backend_.UpsertItem(group_id, input, &error)) {
        owner_.status_.Error("保存条目失败：" + error);
        return;
    }

    owner_.RenderItems();
    owner_.status_.Info(item_id.empty() ? "项目已添加" : "项目已更新");
}
