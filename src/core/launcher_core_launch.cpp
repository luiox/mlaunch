#include "launcher_core.h"

#include <string>
#include <vector>

#include "launcher_core_internal.h"

namespace core {

std::vector<std::string> LauncherBackend::SplitWindowsArgs(const std::string& arguments) {
    std::vector<std::string> out;
    std::string current;
    bool in_quotes = false;
    for (char ch : arguments) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (!in_quotes && (ch == ' ' || ch == '\t')) {
            if (!current.empty()) {
                out.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

LaunchResult LauncherBackend::Launch(const std::string& group_id, const std::string& item_id, std::string* error) {
    LaunchResult result;
    if (!EnsureLoaded(error)) {
        result.message = error ? *error : "backend not loaded";
        return result;
    }

    auto* group = FindGroup(group_id);
    if (!group) {
        SetError(error, "group not found");
        result.message = "group not found";
        return result;
    }
    if (IsRecycleBinId(group_id)) {
        SetError(error, "item is in recycle bin");
        result.message = "item is in recycle bin";
        return result;
    }

    auto it = std::find_if(group->items.begin(), group->items.end(), [&](const LaunchItem& item) { return item.id == item_id; });
    if (it == group->items.end()) {
        SetError(error, "item not found");
        result.message = "item not found";
        return result;
    }

    if (it->item_type == "separator") {
        SetError(error, "separator item cannot be launched");
        result.message = "separator item cannot be launched";
        return result;
    }

    if (Trim(it->target_path).empty()) {
        SetError(error, "target path is empty");
        result.message = "target path is empty";
        return result;
    }

    if (launch_executor_ == nullptr) {
        SetError(error, "no launch executor installed");
        result.message = "no launch executor installed";
        return result;
    }

    // 占位符与环境变量展开（对齐 VB6 Poner：%pr%=程序目录，%cr%=所在盘根目录）。
    std::string launch_target = it->target_path;
    std::string launch_args = it->arguments;
    std::string launch_workdir = it->working_dir;
    if (!app_dir_.empty()) {
        const auto app_dir_text = app_dir_.string();
        const auto drive_root_text = app_dir_.root_path().string();
        ReplaceAllInPlace(&launch_target, "%pr%", app_dir_text);
        ReplaceAllInPlace(&launch_target, "%cr%", drive_root_text);
        ReplaceAllInPlace(&launch_args, "%pr%", app_dir_text);
        ReplaceAllInPlace(&launch_args, "%cr%", drive_root_text);
        ReplaceAllInPlace(&launch_workdir, "%pr%", app_dir_text);
        ReplaceAllInPlace(&launch_workdir, "%cr%", drive_root_text);
    }
    launch_target = ExpandEnvUtf8(launch_target);
    launch_args = ExpandEnvUtf8(launch_args);
    launch_workdir = ExpandEnvUtf8(launch_workdir);

    std::string launch_error;
    if (!launch_executor_->Launch(launch_target, launch_args, launch_workdir, &launch_error)) {
        SetError(error, launch_error.empty() ? "launch failed" : launch_error);
        result.message = "launch failed";
        return result;
    }

    it->launch_count++;
    SaveData(nullptr);

    AppendJournal("launch", "id=" + it->id + " name=" + it->name + " count=" + std::to_string(it->launch_count));

    result.ok = true;
    result.message = "launched: " + it->name;
    return result;
}

} // namespace core
