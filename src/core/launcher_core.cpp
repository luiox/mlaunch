#include "launcher_core.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <vector>

#include "launcher_core_internal.h"

namespace core {

LauncherBackend::LauncherBackend(std::filesystem::path base_dir,
                                std::filesystem::path legacy_root,
                                LaunchExecutor* launch_executor,
                                ShortcutResolver* shortcut_resolver)
    : base_dir_(std::move(base_dir)),
      legacy_root_(std::move(legacy_root)),
      data_path_(base_dir_ / "launcher.v2.json"),
      settings_path_(base_dir_ / "nassistant.settings.json"),
      launch_executor_(launch_executor),
      shortcut_resolver_(shortcut_resolver) {}

void LauncherBackend::SetAppDir(std::filesystem::path dir) {
    app_dir_ = std::move(dir);
}

std::string LauncherBackend::Trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch); });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::string LauncherBackend::ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool LauncherBackend::IsSeparatorItem(const std::string& name, const std::string& target, const std::string& icon) {
    const auto trimmed = Trim(name);
    return (Trim(target).empty() && Trim(icon).empty()) ||
           (trimmed.rfind("----", 0) == 0 && trimmed.size() >= 8 && trimmed.substr(trimmed.size() - 4) == "----");
}

std::string LauncherBackend::GenerateId(const std::string& prefix) {
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return prefix + "_" + std::to_string(now) + "_" + std::to_string(id_counter_++);
}

Group* LauncherBackend::FindGroup(const std::string& group_id) {
    auto it = std::find_if(data_.groups.begin(), data_.groups.end(), [&](const Group& g) { return g.id == group_id; });
    return it == data_.groups.end() ? nullptr : &(*it);
}

const Group* LauncherBackend::FindGroup(const std::string& group_id) const {
    auto it = std::find_if(data_.groups.begin(), data_.groups.end(), [&](const Group& g) { return g.id == group_id; });
    return it == data_.groups.end() ? nullptr : &(*it);
}

bool LauncherBackend::IsRecycleBinId(const std::string& group_id) {
    return group_id == kRecycleBinGroupId;
}

bool LauncherBackend::IsGroupHidden(const std::string& group_id) const {
    const Group* group = FindGroup(group_id);
    return group != nullptr && group->hidden;
}

Group* LauncherBackend::EnsureRecycleBin() {
    if (auto* existing = FindGroup(kRecycleBinGroupId)) {
        return existing;
    }

    int max_order = -1;
    for (const auto& g : data_.groups) {
        if (!g.hidden) {
            max_order = std::max(max_order, g.order);
        }
    }

    Group bin;
    bin.id = kRecycleBinGroupId;
    bin.name = kRecycleBinGroupName;
    bin.order = max_order + 1;
    bin.hidden = true;
    data_.groups.push_back(std::move(bin));
    launcher::log::Info("recycle bin group created");
    return &data_.groups.back();
}

bool LauncherBackend::UndoLastDelete(std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }
    if (!has_last_deleted_) {
        SetError(error, "no deletion to undo");
        return false;
    }

    auto* bin = FindGroup(kRecycleBinGroupId);
    if (bin == nullptr) {
        has_last_deleted_ = false;
        SetError(error, "recycle bin not found");
        return false;
    }

    auto it = std::find_if(bin->items.begin(), bin->items.end(),
        [&](const LaunchItem& item) { return item.id == last_deleted_.item.id; });
    if (it == bin->items.end()) {
        has_last_deleted_ = false;
        SetError(error, "deleted item is no longer in recycle bin");
        return false;
    }

    auto* target = FindGroup(last_deleted_.from_group_id);
    if (target == nullptr) {
        SetError(error, "original group no longer exists");
        return false;
    }

    auto restored = *it;
    bin->items.erase(it);

    const auto index = std::min(last_deleted_.index, target->items.size());
    target->items.insert(target->items.begin() + static_cast<std::ptrdiff_t>(index), std::move(restored));

    AppendJournal("undo_delete", "name=" + last_deleted_.item.name + " group=" + last_deleted_.from_group_name);
    has_last_deleted_ = false;
    return SaveData(error);
}

bool LauncherBackend::SortGroupItemsByName(const std::string& group_id, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }
    if (IsRecycleBinId(group_id)) {
        SetError(error, "recycle bin is managed automatically");
        return false;
    }

    Group* group = FindGroup(group_id);
    if (group == nullptr) {
        SetError(error, "group not found");
        return false;
    }

    // stable_sort：同名条目（如分隔条）保持原有相对顺序。
    std::stable_sort(group->items.begin(), group->items.end(),
        [](const LaunchItem& lhs, const LaunchItem& rhs) {
            return ToLowerAscii(lhs.name) < ToLowerAscii(rhs.name);
        });

    AppendJournal("sort_group", "id=" + group->id + " name=" + group->name + " count=" + std::to_string(group->items.size()));
    return SaveData(error);
}

std::string LauncherBackend::AddGroup(const std::string& name, std::string* error) {
    if (!EnsureLoaded(error)) {
        return {};
    }
    const auto group_name = Trim(name);
    if (group_name.empty()) {
        SetError(error, "group name is empty");
        return {};
    }
    const auto found = std::find_if(data_.groups.begin(), data_.groups.end(), [&](const Group& g) {
        return !g.hidden && ToLowerAscii(g.name) == ToLowerAscii(group_name);
    });
    if (found != data_.groups.end()) {
        SetError(error, "group already exists");
        return {};
    }

    int max_order = -1;
    for (const auto& g : data_.groups) {
        if (!g.hidden) {
            max_order = std::max(max_order, g.order);
        }
    }

    Group g;
    g.id = GenerateId("group");
    g.name = group_name;
    g.order = max_order + 1;
    data_.groups.push_back(g);

    AppendJournal("add_group", "id=" + g.id + " name=" + g.name);
    if (!SaveData(error)) {
        return {};
    }
    return g.id;
}

bool LauncherBackend::RenameGroup(const std::string& group_id, const std::string& name, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }
    const auto next_name = Trim(name);
    if (next_name.empty()) {
        SetError(error, "group name is empty");
        return false;
    }

    const auto conflict = std::find_if(data_.groups.begin(), data_.groups.end(), [&](const Group& g) {
        return !g.hidden && g.id != group_id && ToLowerAscii(g.name) == ToLowerAscii(next_name);
    });
    if (conflict != data_.groups.end()) {
        SetError(error, "group already exists");
        return false;
    }

    auto* group = FindGroup(group_id);
    if (!group) {
        SetError(error, "group not found");
        return false;
    }
    if (group->hidden) {
        SetError(error, "cannot rename recycle bin");
        return false;
    }

    const auto old_name = group->name;
    group->name = next_name;
    AppendJournal("rename_group", "id=" + group_id + " from=" + old_name + " to=" + next_name);
    return SaveData(error);
}

bool LauncherBackend::DeleteGroup(const std::string& group_id, const std::string& target_group_id, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }

    if (data_.groups.size() <= 1) {
        SetError(error, "cannot delete the last group");
        return false;
    }

    if (IsRecycleBinId(group_id)) {
        SetError(error, "cannot delete recycle bin");
        return false;
    }
    if (IsRecycleBinId(target_group_id)) {
        SetError(error, "cannot merge into recycle bin");
        return false;
    }

    auto delete_it = std::find_if(data_.groups.begin(), data_.groups.end(), [&](const Group& g) { return g.id == group_id; });
    if (delete_it == data_.groups.end()) {
        SetError(error, "group not found");
        return false;
    }

    auto target_it = std::find_if(data_.groups.begin(), data_.groups.end(), [&](const Group& g) {
        return g.id == target_group_id && g.id != group_id;
    });
    if (target_it == data_.groups.end()) {
        SetError(error, "target group not found");
        return false;
    }

    std::vector<LaunchItem> moved_items;
    moved_items.reserve(delete_it->items.size());
    for (const auto& item : delete_it->items) {
        moved_items.push_back(item);
    }
    target_it->items.insert(target_it->items.end(), moved_items.begin(), moved_items.end());
    const auto deleted_name = delete_it->name;
    data_.groups.erase(delete_it);

    std::vector<Group*> ordered;
    ordered.reserve(data_.groups.size());
    for (auto& group : data_.groups) {
        ordered.push_back(&group);
    }
    std::sort(ordered.begin(), ordered.end(), [](const Group* lhs, const Group* rhs) {
        return lhs->order < rhs->order;
    });
    for (int i = 0; i < static_cast<int>(ordered.size()); ++i) {
        ordered[i]->order = i;
    }

    if (settings_.current_group.has_value() && *settings_.current_group == group_id) {
        settings_.current_group = target_group_id;
    }

    AppendJournal("delete_group", "id=" + group_id + " name=" + deleted_name + " merged_into=" + target_group_id);
    return SaveData(error);
}

bool LauncherBackend::UpsertItem(const std::string& group_id, const ItemInput& input, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }
    auto* group = FindGroup(group_id);
    if (!group) {
        SetError(error, "group not found");
        return false;
    }
    if (group->hidden) {
        SetError(error, "cannot modify recycle bin directly");
        return false;
    }

    const auto item_type = input.item_type.has_value() ? *input.item_type :
        (IsSeparatorItem(input.name, input.target_path, input.icon_location) ? "separator" : "app");
    const auto target = Trim(input.target_path);
    const auto icon = (item_type == "app" && Trim(input.icon_location).empty()) ? target : Trim(input.icon_location);
    const auto args = Trim(input.arguments);
    const auto name = Trim(input.name);
    const auto enabled = input.enabled.value_or(true);

    if (input.id.has_value()) {
        auto it = std::find_if(group->items.begin(), group->items.end(), [&](const LaunchItem& i) { return i.id == *input.id; });
        if (it != group->items.end()) {
            it->item_type = item_type;
            it->name = name;
            it->target_path = target;
            it->icon_location = icon;
            it->arguments = args;
            it->enabled = enabled;
            AppendJournal("update_item", "id=" + it->id + " name=" + name + " group=" + group->name);
        } else {
            LaunchItem item;
            item.id = *input.id;
            item.item_type = item_type;
            item.name = name;
            item.target_path = target;
            item.icon_location = icon;
            item.arguments = args;
            item.enabled = enabled;
            AppendJournal("add_item", "id=" + item.id + " name=" + name + " group=" + group->name);
            group->items.push_back(std::move(item));
        }
    } else {
        LaunchItem item;
        item.id = GenerateId("item");
        item.item_type = item_type;
        item.name = name;
        item.target_path = target;
        item.icon_location = icon;
        item.arguments = args;
        item.enabled = enabled;
        AppendJournal("add_item", "id=" + item.id + " name=" + name + " group=" + group->name);
        group->items.push_back(std::move(item));
    }

    return SaveData(error);
}

bool LauncherBackend::DeleteItem(const std::string& group_id, const std::string& item_id, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }

    // Deleting inside the recycle bin is the real, permanent delete.
    if (IsRecycleBinId(group_id)) {
        auto* bin = FindGroup(group_id);
        if (!bin) {
            SetError(error, "group not found");
            return false;
        }
        auto item_it = std::find_if(bin->items.begin(), bin->items.end(),
            [&](const LaunchItem& item) { return item.id == item_id; });
        if (item_it == bin->items.end()) {
            SetError(error, "item not found");
            return false;
        }
        const auto purged_name = item_it->name;
        bin->items.erase(item_it);
        if (has_last_deleted_ && last_deleted_.item.id == item_id) {
            has_last_deleted_ = false;
        }
        AppendJournal("purge_item", "id=" + item_id + " name=" + purged_name);
        return SaveData(error);
    }

    // Soft delete: move into the built-in recycle bin group.
    // EnsureRecycleBin may grow data_.groups, so (re)acquire the source group after it.
    auto* bin = EnsureRecycleBin();
    auto* group = FindGroup(group_id);
    if (!group) {
        SetError(error, "group not found");
        return false;
    }
    if (group->hidden) {
        SetError(error, "cannot modify hidden groups directly");
        return false;
    }

    auto item_it = std::find_if(group->items.begin(), group->items.end(),
        [&](const LaunchItem& item) { return item.id == item_id; });
    if (item_it == group->items.end()) {
        SetError(error, "item not found");
        return false;
    }

    DeletedItemSnapshot snapshot;
    snapshot.item = *item_it;
    snapshot.from_group_id = group->id;
    snapshot.from_group_name = group->name;
    snapshot.index = static_cast<std::size_t>(std::distance(group->items.begin(), item_it));

    group->items.erase(item_it);
    bin->items.push_back(snapshot.item);

    last_deleted_ = std::move(snapshot);
    has_last_deleted_ = true;

    AppendJournal("delete_item", "id=" + last_deleted_.item.id + " name=" + last_deleted_.item.name +
        " from=" + last_deleted_.from_group_name + " to=recycle_bin");
    return SaveData(error);
}

bool LauncherBackend::MoveItem(const std::string& group_id, const std::string& item_id, const std::string& target_group_id, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }
    if (group_id == target_group_id) {
        SetError(error, "source and target group are the same");
        return false;
    }
    if (IsRecycleBinId(group_id)) {
        SetError(error, "cannot move out of recycle bin; use undo");
        return false;
    }
    if (IsRecycleBinId(target_group_id)) {
        SetError(error, "cannot move into recycle bin; use delete");
        return false;
    }

    auto* from = FindGroup(group_id);
    auto* to = FindGroup(target_group_id);
    if (!from) {
        SetError(error, "source group not found");
        return false;
    }
    if (!to) {
        SetError(error, "target group not found");
        return false;
    }

    auto it = std::find_if(from->items.begin(), from->items.end(), [&](const LaunchItem& item) { return item.id == item_id; });
    if (it == from->items.end()) {
        SetError(error, "item not found");
        return false;
    }

    auto moved = *it;
    from->items.erase(it);
    to->items.push_back(std::move(moved));

    AppendJournal("move_item", "id=" + item_id + " from=" + from->name + " to=" + to->name);
    return SaveData(error);
}

bool LauncherBackend::ReorderGroup(const std::string& group_id, int target_index, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }
    if (IsRecycleBinId(group_id)) {
        SetError(error, "cannot reorder recycle bin");
        return false;
    }
    if (data_.groups.empty()) {
        SetError(error, "group list is empty");
        return false;
    }

    std::vector<Group*> ordered;
    ordered.reserve(data_.groups.size());
    for (auto& group : data_.groups) {
        if (!group.hidden) {
            ordered.push_back(&group);
        }
    }
    std::sort(ordered.begin(), ordered.end(), [](const Group* lhs, const Group* rhs) {
        return lhs->order < rhs->order;
    });

    const auto from_it = std::find_if(ordered.begin(), ordered.end(), [&](const Group* group) {
        return group->id == group_id;
    });
    if (from_it == ordered.end()) {
        SetError(error, "group not found");
        return false;
    }

    const int count = static_cast<int>(ordered.size());
    if (target_index < 0 || target_index >= count) {
        SetError(error, "target index out of range");
        return false;
    }

    const int from_index = static_cast<int>(std::distance(ordered.begin(), from_it));
    if (from_index == target_index) {
        return true;
    }

    if (from_index < target_index) {
        std::rotate(ordered.begin() + from_index, ordered.begin() + from_index + 1, ordered.begin() + target_index + 1);
    } else {
        std::rotate(ordered.begin() + target_index, ordered.begin() + from_index, ordered.begin() + from_index + 1);
    }

    for (int i = 0; i < static_cast<int>(ordered.size()); ++i) {
        ordered[i]->order = i;
    }

    AppendJournal("reorder_group", "id=" + group_id + " to_index=" + std::to_string(target_index));
    return SaveData(error);
}

bool LauncherBackend::ReorderItemInGroup(const std::string& group_id, const std::string& item_id, int target_index, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }

    auto* group = FindGroup(group_id);
    if (!group) {
        SetError(error, "group not found");
        return false;
    }

    const int count = static_cast<int>(group->items.size());
    if (count == 0) {
        SetError(error, "item list is empty");
        return false;
    }
    if (target_index < 0 || target_index >= count) {
        SetError(error, "target index out of range");
        return false;
    }

    const auto from_it = std::find_if(group->items.begin(), group->items.end(), [&](const LaunchItem& item) {
        return item.id == item_id;
    });
    if (from_it == group->items.end()) {
        SetError(error, "item not found");
        return false;
    }

    const int from_index = static_cast<int>(std::distance(group->items.begin(), from_it));
    if (from_index == target_index) {
        return true;
    }

    if (from_index < target_index) {
        std::rotate(group->items.begin() + from_index, group->items.begin() + from_index + 1, group->items.begin() + target_index + 1);
    } else {
        std::rotate(group->items.begin() + target_index, group->items.begin() + from_index, group->items.begin() + from_index + 1);
    }

    AppendJournal("reorder_item", "id=" + item_id + " group=" + group->name + " to_index=" + std::to_string(target_index));
    return SaveData(error);
}

std::string LauncherBackend::BasenameNoExt(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

std::string LauncherBackend::PercentDecodePath(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 2 < input.size()) {
            const auto hex = input.substr(i + 1, 2);
            char* end = nullptr;
            const auto val = static_cast<char>(std::strtol(hex.c_str(), &end, 16));
            if (end && *end == '\0') {
                out.push_back(val);
                i += 2;
                continue;
            }
        }
        out.push_back(input[i]);
    }
    return out;
}

std::string LauncherBackend::NormalizeDroppedPath(const std::string& raw_path) {
    auto value = Trim(raw_path);
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }

    auto lower = ToLowerAscii(value);
    if (lower.rfind("file://", 0) == 0) {
        if (lower.rfind("file:///", 0) == 0) {
            value = value.substr(8);
        } else {
            value = value.substr(7);
        }
        std::replace(value.begin(), value.end(), '/', '\\');
        value = PercentDecodePath(value);
    }

    return value;
}

std::size_t LauncherBackend::CreateItemsFromDroppedPaths(const std::string& group_id, const std::vector<std::string>& paths, std::string* error) {
    if (!EnsureLoaded(error)) {
        return 0;
    }

    auto* group = FindGroup(group_id);
    if (!group) {
        SetError(error, "group not found");
        return 0;
    }
    if (IsRecycleBinId(group_id)) {
        SetError(error, "cannot drop into recycle bin");
        return 0;
    }

    std::size_t created = 0;
    for (const auto& raw : paths) {
        auto dropped = NormalizeDroppedPath(raw);
        if (dropped.empty()) {
            continue;
        }

        std::string target = dropped;
        std::string args;

        auto lower = ToLowerAscii(dropped);
        if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".lnk") {
            if (shortcut_resolver_ == nullptr) {
                continue;
            }
            auto resolved = shortcut_resolver_->Resolve(dropped);
            if (!resolved.has_value()) {
                continue;
            }
            target = Trim(resolved->first);
            args = Trim(resolved->second);
            if (target.empty()) {
                continue;
            }
        }

        auto name = BasenameNoExt(target);
        if (name.empty()) {
            continue;
        }

        LaunchItem item;
        item.id = GenerateId("item");
        item.item_type = "app";
        item.name = name;
        item.target_path = target;
        item.icon_location = target;
        item.arguments = args;
        item.launch_count = 0;
        item.enabled = true;
        group->items.push_back(std::move(item));
        ++created;
    }

    if (created > 0) {
        AppendJournal("import_drop", "count=" + std::to_string(created) + " group=" + group->name);
        SaveData(nullptr);
    }
    return created;
}

} // namespace core
