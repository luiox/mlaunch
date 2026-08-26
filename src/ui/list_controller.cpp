#include "list_controller.h"

#include <algorithm>

#include "app_window.h"
#include "file_icon_control.h"
#include "search_controller.h"
#include "ui_controls.h"
#include "utils/string_util.h"

using namespace DuiLib;

ListController::ListController(AppWindow& owner)
    : owner_(owner) {}

void ListController::RenderGroups() {
    if (!owner_.groups_list_) {
        return;
    }
    owner_.groups_list_->RemoveAll();
    owner_.group_ids_.clear();

    std::vector<const core::Group*> groups;
    const core::Group* recycle_bin = nullptr;
    groups.reserve(owner_.backend_.Data().groups.size());
    for (const auto& group : owner_.backend_.Data().groups) {
        if (group.hidden) {
            if (core::LauncherBackend::IsRecycleBinId(group.id)) {
                recycle_bin = &group;
            }
            continue;
        }
        groups.push_back(&group);
    }
    std::sort(groups.begin(), groups.end(), [](const core::Group* lhs, const core::Group* rhs) {
        return lhs->order < rhs->order;
    });
    if (recycle_bin != nullptr) {
        groups.push_back(recycle_bin);
    }

    for (const auto* group : groups) {
        auto* row = new appui::GroupRowUI();
        // 参考图分组行距 33px（选中条 32px 描边带 + 1px 行距），明显高于 28px 的条目行。
        row->SetFixedHeight(33);
        // 选中/悬停高亮由 GroupListUI 列表级 item 色控制（对齐 Poner 灰阶）。

        std::wstring display_name;
        if (group->hidden && core::LauncherBackend::IsRecycleBinId(group->id)) {
            // 数据文件里的组名可能是旧英文常量，显示层统一为中文。
            display_name = L"回收站";
        } else {
            display_name = launcher::util::Utf8ToWide(group->name);
        }
        if (group->hidden) {
            display_name += L" (" + std::to_wstring(group->items.size()) + L")";
        }

        auto* name = new CLabelUI();
        name->SetText(display_name.c_str());
        // 对齐参考图采样：分组名在面板内水平居中（Common/IDE 等中心 ≈ 面板中心）。
        name->SetTextColor(0xFF1A1A1A);
        name->SetTextStyle(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        row->Add(name);

        owner_.groups_list_->Add(row);
        owner_.group_ids_.push_back(group->id);
    }
}

void ListController::RenderItems() {
    if (!owner_.items_list_) {
        return;
    }

    owner_.items_list_->RemoveAll();
    owner_.item_ids_.clear();
    owner_.item_group_ids_.clear();
    owner_.selected_item_id_.clear();
    owner_.selected_item_group_id_.clear();

    if (owner_.search_mode_) {
        std::string keyword;
        if (owner_.search_input_ != nullptr) {
            keyword = launcher::util::WideToUtf8(owner_.search_input_->GetText().GetData());
        }

        const int active_cmd = owner_.search_controller_.GetActiveCommand();

        // 对齐原版：空输入不显示任何结果，改为三行居中提示。
        if (keyword.empty() && active_cmd == launcher::constants::search_cmd::kNone) {
            int list_height = owner_.items_list_->GetPos().bottom - owner_.items_list_->GetPos().top;
            int spacer = list_height / 2 - 60;
            if (spacer < 0) {
                spacer = 0;
            }
            auto* spacer_row = new CListContainerElementUI();
            spacer_row->SetFixedHeight(spacer);
            spacer_row->SetEnabled(false);
            owner_.items_list_->Add(spacer_row);
            owner_.item_ids_.push_back("");
            owner_.item_group_ids_.push_back("");

            const std::wstring hints[] = {
                L"输入关键字开始搜索，ESC键退出搜索",
                L"上下键选择，回车键运行",
                L"如果你觉得 Poner 不错就介绍给朋友吧",
            };
            for (const auto& hint : hints) {
                auto* row = new CListContainerElementUI();
                row->SetFixedHeight(33);
                row->SetEnabled(false);
                row->SetAttribute(_T("inset"), _T("4,0,4,0"));

                auto* label = new CLabelUI();
                label->SetText(hint.c_str());
                label->SetTextColor(0xFF808689);
                label->SetTextStyle(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                row->Add(label);

                owner_.items_list_->Add(row);
                owner_.item_ids_.push_back("");
                owner_.item_group_ids_.push_back("");
            }
            return;
        }

        if (active_cmd != launcher::constants::search_cmd::kNone) {
            auto add_command_row = [&](const std::wstring& label, const std::wstring& desc, int cmd_id) {
                auto* row = new CListContainerElementUI();
                row->SetFixedHeight(28);
                row->SetAttribute(_T("inset"), _T("4,0,4,0"));
                row->SetAttribute(_T("childpadding"), _T("10"));
                row->SetAttribute(_T("childvalign"), _T("vcenter"));

                auto* icon = new FileIconControl();
                icon->SetFixedWidth(20);
                icon->SetFixedHeight(20);
                row->Add(icon);

                auto* text_layout = new CVerticalLayoutUI();
                text_layout->SetAttribute(_T("childpadding"), _T("0"));

                auto* name = new CLabelUI();
                name->SetText(label.c_str());
                name->SetTextColor(0xFF1A73E8);
                name->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                text_layout->Add(name);

                if (!desc.empty()) {
                    auto* desc_label = new CLabelUI();
                    desc_label->SetText(desc.c_str());
                    desc_label->SetTextColor(0xFF808689);
                    desc_label->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                    desc_label->SetAttribute(_T("font"), _T("1"));
                    text_layout->Add(desc_label);
                }

                row->Add(text_layout);

                owner_.items_list_->Add(row);
                owner_.item_ids_.push_back(SearchController::CommandIdToItemId(cmd_id));
                owner_.item_group_ids_.push_back("");
            };

            switch (active_cmd) {
            case launcher::constants::search_cmd::kCmd:
                add_command_row(L"打开命令提示符", L"cmd", active_cmd);
                break;
            case launcher::constants::search_cmd::kSettings:
                add_command_row(L"打开系统设置", L"setting", active_cmd);
                break;
            case launcher::constants::search_cmd::kShutdown:
                add_command_row(L"关闭计算机", L"shutdown", active_cmd);
                break;
            case launcher::constants::search_cmd::kReboot:
                add_command_row(L"重启计算机", L"reboot", active_cmd);
                break;
            case launcher::constants::search_cmd::kLogoff:
                add_command_row(L"注销当前用户", L"logoff", active_cmd);
                break;
            case launcher::constants::search_cmd::kScreenoff:
                add_command_row(L"关闭显示器", L"screenoff", active_cmd);
                break;
            case launcher::constants::search_cmd::kBaidu:
                add_command_row(L"百度搜索", launcher::util::Utf8ToWide(owner_.search_controller_.GetBaiduKeyword()), active_cmd);
                break;
            }
            return;
        }

        for (const auto& group : owner_.backend_.Data().groups) {
            if (group.hidden) {
                continue;
            }
            for (const auto& item : group.items) {
                if (item.item_type == "separator") {
                    continue;
                }
                if (!keyword.empty() && !owner_.ContainsCaseInsensitive(item.name, keyword)) {
                    continue;
                }

                auto* row = new CListContainerElementUI();
                row->SetFixedHeight(28);
                row->SetAttribute(_T("inset"), _T("4,0,4,0"));
                row->SetAttribute(_T("childpadding"), _T("10"));
                row->SetAttribute(_T("childvalign"), _T("vcenter"));

                auto* icon = new FileIconControl();
                icon->SetFixedWidth(20);
                icon->SetFixedHeight(20);
                icon->SetIconPath(launcher::util::Utf8ToWide(owner_.icon_manager_.ParseItemIconSource(item)));
                row->Add(icon);

                auto* name = new CLabelUI();
                name->SetText(launcher::util::Utf8ToWide(item.name + "  [" + group.name + "]").c_str());
                name->SetTextColor(0xFF1A1A1A);
                name->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                row->Add(name);

                owner_.items_list_->Add(row);
                owner_.item_ids_.push_back(item.id);
                owner_.item_group_ids_.push_back(group.id);
            }
        }
        return;
    }

    const core::Group* group = owner_.FindActiveGroup();
    if (!group) {
        return;
    }

    for (const auto& item : group->items) {
        if (item.item_type == "separator") {
            auto* row = new CListContainerElementUI();
            row->SetFixedHeight(28);

            auto* name = new CLabelUI();
            name->SetText(launcher::util::Utf8ToWide(item.name).c_str());
            name->SetAttribute(_T("padding"), _T("10,0,0,0"));
            name->SetTextColor(0xFF909090);
            name->SetTextStyle(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            row->Add(name);

            owner_.items_list_->Add(row);
            owner_.item_ids_.push_back(item.id);
            owner_.item_group_ids_.push_back(group->id);
            continue;
        }

        auto* row = new CListContainerElementUI();
        row->SetFixedHeight(28);
        row->SetAttribute(_T("inset"), _T("4,0,4,0"));
        row->SetAttribute(_T("childpadding"), _T("10"));
        row->SetAttribute(_T("childvalign"), _T("vcenter"));

        auto* icon = new FileIconControl();
        icon->SetFixedWidth(20);
        icon->SetFixedHeight(20);
        icon->SetIconPath(launcher::util::Utf8ToWide(owner_.icon_manager_.ParseItemIconSource(item)));
        row->Add(icon);

        auto* name = new CLabelUI();
        name->SetText(launcher::util::Utf8ToWide(item.name).c_str());
        name->SetTextColor(0xFF1A1A1A);
        name->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        row->Add(name);

        owner_.items_list_->Add(row);
        owner_.item_ids_.push_back(item.id);
        owner_.item_group_ids_.push_back(group->id);
    }
}

void ListController::SelectGroupByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(owner_.group_ids_.size())) {
        return;
    }
    owner_.active_group_id_ = owner_.group_ids_[index];
    if (owner_.groups_list_) {
        owner_.groups_list_->SelectItem(index, false);
    }
    RenderItems();
}

bool ListController::SelectListRowFromPoint(CListUI* list, const std::vector<std::string>& ids, const POINT& client_point, std::string* selected_id) {
    if (list == nullptr) {
        return false;
    }
    const RECT list_rect = list->GetPos();
    if (!PtInRect(&list_rect, client_point)) {
        return false;
    }

    for (int i = 0; i < list->GetCount(); ++i) {
        CControlUI* item = list->GetItemAt(i);
        if (item == nullptr) {
            continue;
        }
        const RECT row_rect = item->GetPos();
        if (PtInRect(&row_rect, client_point)) {
            list->SelectItem(i, false);
            if (selected_id != nullptr && i >= 0 && i < static_cast<int>(ids.size())) {
                *selected_id = ids[i];
            }
            return true;
        }
    }
    return true;
}
