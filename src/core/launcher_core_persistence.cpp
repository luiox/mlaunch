#include "launcher_core.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <map>

#include "launcher_core_internal.h"

namespace core {

LauncherData LauncherBackend::DefaultLauncherData() const {
    LauncherData data;
    data.version = 2;
    Group group;
    group.id = const_cast<LauncherBackend*>(this)->GenerateId("group");
    group.name = "Common";
    group.order = 0;
    data.groups.push_back(group);
    return data;
}

Settings LauncherBackend::ParsePonerCfg(const std::filesystem::path& cfg_path) const {
    Settings settings;
    std::ifstream in(cfg_path);
    if (!in) {
        return settings;
    }

    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(in, line)) {
        auto trimmed = launcher::util::Trim(line);
        if (trimmed.empty() || trimmed[0] == '[') {
            continue;
        }
        const auto pos = trimmed.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        values[launcher::util::Trim(trimmed.substr(0, pos))] = launcher::util::Trim(trimmed.substr(pos + 1));
    }

    const auto it_hide = values.find("ExecuteHide");
    if (it_hide != values.end()) {
        auto value = ToLowerAscii(it_hide->second);
        settings.execute_hide = (value == "1" || value == "true");
    }

    auto it_current = values.find("CurrentTab");
    if (it_current == values.end()) {
        it_current = values.find("ActivatedTab");
    }
    if (it_current != values.end() && !launcher::util::Trim(it_current->second).empty()) {
        settings.current_group = it_current->second;
    }

    return settings;
}

bool LauncherBackend::Load(std::string* error) {
    static constexpr int kSupportedDataVersion = 2;

    std::error_code ec;
    std::filesystem::create_directories(base_dir_, ec);

    // 损坏/版本不兼容的统一恢复路径：备份原文件、移除、回退默认数据。
    auto recover_with_defaults = [&]() {
        BackupCorruptedJson(data_path_);
        // Remove the unusable file so the upcoming default-save does not
        // rotate corrupted content into the backup directory.
        std::filesystem::remove(data_path_, ec);
        last_load_corrupted_ = true;
        data_ = DefaultLauncherData();
        data_.version = kSupportedDataVersion;
        return SaveData(error);
    };

    if (std::filesystem::exists(data_path_)) {
        auto raw = ReadTextFile(data_path_);
        if (!raw.empty()) {
            std::string parse_error;
            auto parsed = ParseJsonText(raw, &parse_error);
            if (!parsed) {
                launcher::log::Error("parse launcher.v2.json failed: " + parse_error);
                if (!recover_with_defaults()) {
                    return false;
                }
                loaded_ = true;
                return true;
            }

            ca::json::JsonDocument doc = std::move(*parsed);
            const auto& root = doc.root();

            data_.version = GetInt(root, "version", kSupportedDataVersion);
            if (data_.version != kSupportedDataVersion) {
                launcher::log::Warn("incompatible launcher.v2.json version=" + std::to_string(data_.version));
                if (!recover_with_defaults()) {
                    return false;
                }
                loaded_ = true;
                return true;
            }

            data_.groups.clear();
            const auto* groups = FindField(root, "groups");
            if (groups == nullptr || !groups->is_array()) {
                data_ = DefaultLauncherData();
            } else {
                for (const auto& jg : groups->as_array()) {
                    Group g;
                    g.id = GetStr(jg, "id", GenerateId("group"));
                    g.name = GetStr(jg, "name", std::string("Common"));
                    g.order = GetInt(jg, "order", 0);
                    g.hidden = GetBool(jg, "hidden", false);
                    if (const auto* items = FindField(jg, "items"); items != nullptr && items->is_array()) {
                        for (const auto& ji : items->as_array()) {
                            LaunchItem item;
                            item.id = GetStr(ji, "id", GenerateId("item"));
                            item.item_type = GetStr(ji, "itemType", std::string("app"));
                            item.name = GetStr(ji, "name", std::string());
                            item.target_path = GetStr(ji, "targetPath", std::string());
                            item.icon_location = GetStr(ji, "iconLocation", std::string());
                            item.working_dir = GetStr(ji, "workingDir", std::string());
                            item.arguments = GetStr(ji, "arguments", std::string());
                            item.launch_count = GetU64(ji, "launchCount", 0);
                            item.enabled = GetBool(ji, "enabled", true);
                            g.items.push_back(std::move(item));
                        }
                    }
                    data_.groups.push_back(std::move(g));
                }
            }
            if (data_.groups.empty()) {
                data_ = DefaultLauncherData();
                if (!SaveData(error)) {
                    return false;
                }
            }
        }
    } else {
        const auto legacy_data_path = legacy_root_ / "Data.json";
        if (std::filesystem::exists(legacy_data_path)) {
            auto raw = ReadTextFile(legacy_data_path);
            std::string parse_error;
            auto parsed = ParseJsonText(raw, &parse_error);
            if (!parsed) {
                launcher::log::Error("parse legacy Data.json failed: " + parse_error);
                SetError(error, std::string("parse legacy Data.json failed: ") + parse_error);
                return false;
            }

            ca::json::JsonDocument doc = std::move(*parsed);
            const auto& legacy = doc.root();

            data_.version = 2;
            data_.groups.clear();
            int order = 0;
            if (legacy.is_object()) {
                for (const auto& member : legacy.as_object()) {
                    Group group;
                    group.id = GenerateId("group");
                    group.name = ToStdString(member.first);
                    group.order = order++;

                    if (member.second.is_array()) {
                        for (const auto& ji : member.second.as_array()) {
                            LaunchItem item;
                            item.id = GenerateId("item");
                            item.name = GetStr(ji, "Name", std::string());
                            item.target_path = GetStr(ji, "TargetPath", std::string());
                            item.icon_location = GetStr(ji, "IconLocation", std::string());
                            item.arguments = GetStr(ji, "Arguments", std::string());
                            item.launch_count = GetU64(ji, "Count", 0);
                            item.item_type = IsSeparatorItem(item.name, item.target_path, item.icon_location) ? "separator" : "app";
                            item.enabled = true;
                            group.items.push_back(std::move(item));
                        }
                    }
                    data_.groups.push_back(std::move(group));
                }
            }

            if (data_.groups.empty()) {
                data_ = DefaultLauncherData();
            }

            if (!SaveData(error)) {
                return false;
            }
        } else {
            data_ = DefaultLauncherData();
            if (!SaveData(error)) {
                return false;
            }
        }
    }

    if (std::filesystem::exists(settings_path_)) {
        auto raw = ReadTextFile(settings_path_);
        if (!raw.empty()) {
            std::string parse_error;
            auto parsed = ParseJsonText(raw, &parse_error);
            if (!parsed) {
                SetError(error, std::string("parse settings failed: ") + parse_error);
                return false;
            }
            ca::json::JsonDocument doc = std::move(*parsed);
            const auto& root = doc.root();
            settings_.title = GetStr(root, "title", std::string("mlaunch"));
            settings_.hotkey = GetStr(root, "hotkey", std::string("Alt+1"));
            settings_.execute_hide = GetBool(root, "executeHide", true);
            settings_.locked = GetBool(root, "locked", false);
            settings_.auto_hide = GetBool(root, "autoHide", false);
            settings_.autorun = GetBool(root, "autorun", false);
            settings_.start_hidden = GetBool(root, "startHidden", false);
            settings_.close_minimize = GetBool(root, "closeMinimize", false);
            settings_.double_click_launch = GetBool(root, "doubleClickLaunch", false);
            settings_.backup_rolling_count = static_cast<int>(GetInt(root, "backupRollingCount", 5));
            settings_.backup_daily_days = static_cast<int>(GetInt(root, "backupDailyDays", 30));
            if (const auto* cg = FindField(root, "currentGroup"); cg != nullptr && cg->is_string()) {
                settings_.current_group = ToStdString(cg->as_string());
            }
            settings_.group_panel_width = GetF64(root, "groupPanelWidth", 220.0);
            settings_.main_window_width = GetF64(root, "mainWindowWidth", 1040.0);
            settings_.main_window_height = GetF64(root, "mainWindowHeight", 700.0);
            if (launcher::util::Trim(settings_.title).empty()) {
                settings_.title = "mlaunch";
            }
        }
    } else {
        const auto cfg = legacy_root_ / "Poner.cfg";
        if (std::filesystem::exists(cfg)) {
            settings_ = ParsePonerCfg(cfg);
        }
        if (!SaveSettings(error)) {
            return false;
        }
    }

    loaded_ = true;
    return true;
}

std::string LauncherBackend::SerializeCurrentData() const {
    ca::json::JsonDocument doc;
    auto& arena = doc.arena();
    ca::json::JsonValue root = ca::json::JsonValue::make_object();
    SetInt(root, arena, "version", data_.version);

    ca::json::JsonValue groups = ca::json::JsonValue::make_array();
    for (const auto& g : data_.groups) {
        ca::json::JsonValue jg = ca::json::JsonValue::make_object();
        SetStr(jg, arena, "id", g.id);
        SetStr(jg, arena, "name", g.name);
        SetInt(jg, arena, "order", g.order);
        SetBool(jg, arena, "hidden", g.hidden);
        ca::json::JsonValue items = ca::json::JsonValue::make_array();
        for (const auto& i : g.items) {
            ca::json::JsonValue ji = ca::json::JsonValue::make_object();
            SetStr(ji, arena, "id", i.id);
            SetStr(ji, arena, "itemType", i.item_type);
            SetStr(ji, arena, "name", i.name);
            SetStr(ji, arena, "targetPath", i.target_path);
            SetStr(ji, arena, "iconLocation", i.icon_location);
            SetStr(ji, arena, "workingDir", i.working_dir);
            SetStr(ji, arena, "arguments", i.arguments);
            SetInt(ji, arena, "launchCount", static_cast<std::int64_t>(i.launch_count));
            SetBool(ji, arena, "enabled", i.enabled);
            items.append(std::move(ji));
        }
        jg.set(arena.intern("items"), std::move(items));
        groups.append(std::move(jg));
    }
    root.set(arena.intern("groups"), std::move(groups));
    doc.root() = std::move(root);

    return SerializeDocument(doc);
}

bool LauncherBackend::SaveData(std::string* error) const {
    RotateBackupsBeforeSave();
    return WriteTextAtomic(data_path_, SerializeCurrentData(), error);
}

bool LauncherBackend::SaveSettings(std::string* error) const {
    ca::json::JsonDocument doc;
    auto& arena = doc.arena();
    ca::json::JsonValue root = ca::json::JsonValue::make_object();
    SetStr(root, arena, "title", settings_.title);
    SetStr(root, arena, "hotkey", settings_.hotkey);
    SetBool(root, arena, "executeHide", settings_.execute_hide);
    SetBool(root, arena, "locked", settings_.locked);
    SetBool(root, arena, "autoHide", settings_.auto_hide);
    SetBool(root, arena, "autorun", settings_.autorun);
    SetBool(root, arena, "startHidden", settings_.start_hidden);
    SetBool(root, arena, "closeMinimize", settings_.close_minimize);
    SetBool(root, arena, "doubleClickLaunch", settings_.double_click_launch);
    SetF64(root, arena, "backupRollingCount", static_cast<double>(settings_.backup_rolling_count));
    SetF64(root, arena, "backupDailyDays", static_cast<double>(settings_.backup_daily_days));
    if (settings_.current_group.has_value()) {
        SetStr(root, arena, "currentGroup", *settings_.current_group);
    } else {
        root.set(arena.intern("currentGroup"), ca::json::JsonValue::make_null());
    }
    SetF64(root, arena, "groupPanelWidth", settings_.group_panel_width);
    SetF64(root, arena, "mainWindowWidth", settings_.main_window_width);
    SetF64(root, arena, "mainWindowHeight", settings_.main_window_height);
    doc.root() = std::move(root);

    return WriteTextAtomic(settings_path_, SerializeDocument(doc), error);
}

bool LauncherBackend::EnsureLoaded(std::string* error) const {
    if (!loaded_) {
        SetError(error, "backend not loaded");
        return false;
    }
    return true;
}

void LauncherBackend::RotateBackupsBeforeSave() const {
    std::error_code ec;
    if (!std::filesystem::exists(data_path_, ec)) {
        return;
    }

    const auto backup_dir = base_dir_ / "backups";
    std::filesystem::create_directories(backup_dir, ec);

    const auto now = std::chrono::system_clock::now();

    auto copy_backup = [&](const std::string& file_name) {
        std::error_code copy_ec;
        std::filesystem::copy_file(data_path_, backup_dir / file_name,
            std::filesystem::copy_options::overwrite_existing, copy_ec);
        if (copy_ec) {
            launcher::log::Warn("backup copy failed: " + file_name + " error=" + copy_ec.message());
        }
    };

    copy_backup("launcher.v2." + FormatFileStamp(now) + ".json");
    copy_backup("launcher.v2." + FormatDateStamp(now) + ".json");

    PruneBackups(backup_dir,
                 static_cast<std::size_t>(settings_.backup_rolling_count),
                 static_cast<std::size_t>(settings_.backup_daily_days));
}

void LauncherBackend::AppendJournal(const std::string& action, const std::string& detail) const {
    std::error_code ec;
    std::filesystem::create_directories(base_dir_, ec);

    std::ofstream out(base_dir_ / "operations.log", std::ios::app | std::ios::binary);
    if (!out) {
        launcher::log::Warn("open operations.log failed");
        return;
    }
    out << FormatJournalTimestamp(std::chrono::system_clock::now()) << " | " << action << " | " << detail << "\n";
    if (!out.good()) {
        launcher::log::Warn("write operations.log failed");
    }
}

std::size_t LauncherBackend::ImportPonerData(const std::filesystem::path& legacy_json_path, std::string* error) {
    if (!EnsureLoaded(error)) {
        return 0;
    }

    std::error_code ec;
    if (!std::filesystem::exists(legacy_json_path, ec)) {
        SetError(error, "legacy data file not found");
        return 0;
    }

    // 安全层：导入前显式快照当前数据（rolling 命名，随轮转自然淘汰）。
    RotateBackupsBeforeSave();

    auto raw = ReadTextFile(legacy_json_path);
    std::string parse_error;
    auto parsed = ParseJsonText(raw, &parse_error);
    if (!parsed) {
        SetError(error, "parse legacy Data.json failed: " + parse_error);
        return 0;
    }
    ca::json::JsonDocument doc = std::move(*parsed);
    const auto& legacy = doc.root();
    if (!legacy.is_object()) {
        SetError(error, "legacy Data.json format invalid");
        return 0;
    }

    int next_order = 0;
    for (const auto& g : data_.groups) {
        if (!g.hidden) {
            next_order = std::max(next_order, g.order + 1);
        }
    }

    std::size_t merged = 0;
    for (const auto& member : legacy.as_object()) {
        const auto group_name = ToStdString(member.first);
        if (group_name.empty() || !member.second.is_array()) {
            continue;
        }

        // 组按名称匹配（忽略大小写，跳过隐藏分组），缺失则追加到末尾。
        Group* target_group = nullptr;
        for (auto& g : data_.groups) {
            if (!g.hidden && ToLowerAscii(g.name) == ToLowerAscii(group_name)) {
                target_group = &g;
                break;
            }
        }
        if (target_group == nullptr) {
            Group g;
            g.id = GenerateId("group");
            g.name = group_name;
            g.order = next_order++;
            data_.groups.push_back(std::move(g));
            target_group = &data_.groups.back();
        }

        for (const auto& ji : member.second.as_array()) {
            LaunchItem incoming;
            incoming.name = GetStr(ji, "Name", std::string());
            incoming.target_path = GetStr(ji, "TargetPath", std::string());
            incoming.icon_location = GetStr(ji, "IconLocation", std::string());
            incoming.arguments = GetStr(ji, "Arguments", std::string());
            incoming.launch_count = GetU64(ji, "Count", 0);
            incoming.item_type = IsSeparatorItem(incoming.name, incoming.target_path, incoming.icon_location) ? "separator" : "app";
            incoming.enabled = true;

            if (incoming.item_type == "separator") {
                // 分隔条目无稳定 TargetPath，按名称去重保持幂等。
                bool exists = false;
                for (const auto& item : target_group->items) {
                    if (item.item_type == "separator" && item.name == incoming.name) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    LaunchItem item = incoming;
                    item.id = GenerateId("item");
                    target_group->items.push_back(std::move(item));
                    ++merged;
                }
                continue;
            }

            // 幂等键: (组名, TargetPath)，路径忽略大小写。
            const auto target_key = ToLowerAscii(incoming.target_path);
            LaunchItem* existing = nullptr;
            for (auto& item : target_group->items) {
                if (item.item_type != "separator" && ToLowerAscii(item.target_path) == target_key) {
                    existing = &item;
                    break;
                }
            }

            if (existing != nullptr) {
                // 已存在：Count/名称以 Poner 为准，其余保留本地自定义。
                existing->launch_count = incoming.launch_count;
                if (!incoming.name.empty()) {
                    existing->name = incoming.name;
                }
                ++merged;
            } else {
                LaunchItem item = incoming;
                item.id = GenerateId("item");
                target_group->items.push_back(std::move(item));
                ++merged;
            }
        }
    }

    if (merged > 0) {
        AppendJournal("import_poner", "file=" + legacy_json_path.filename().string() + " merged=" + std::to_string(merged));
        if (!SaveData(error)) {
            return 0;
        }
    }
    return merged;
}

bool LauncherBackend::ExportData(const std::filesystem::path& target_path, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }
    if (target_path.empty()) {
        SetError(error, "export path is empty");
        return false;
    }

    std::error_code ec;
    if (target_path.has_parent_path()) {
        std::filesystem::create_directories(target_path.parent_path(), ec);
    }

    // 导出属于可回滚动作（只新增外部副本，不触碰当前数据），不走备份轮转。
    if (!WriteTextAtomic(target_path, SerializeCurrentData(), error)) {
        return false;
    }

    AppendJournal("export_data", "to=" + target_path.string());
    return true;
}

bool LauncherBackend::UpdateSettings(const Settings& settings, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }

    Settings next = settings;
    next.title = launcher::util::Trim(next.title);
    if (next.title.empty()) {
        next.title = "mlaunch";
    }
    next.hotkey = launcher::util::Trim(next.hotkey);
    next.group_panel_width = std::clamp(next.group_panel_width,
        static_cast<double>(limits::kGroupPanelWidthMin), static_cast<double>(limits::kGroupPanelWidthMax));
    next.main_window_width = std::max(next.main_window_width,
                                      static_cast<double>(limits::kMainWindowWidthMin));
    next.main_window_height = std::max(next.main_window_height,
                                       static_cast<double>(limits::kMainWindowHeightMin));
    next.backup_rolling_count = std::clamp(next.backup_rolling_count,
                                           limits::kBackupRollingMin, limits::kBackupRollingMax);
    next.backup_daily_days = std::clamp(next.backup_daily_days,
                                        limits::kBackupDailyMin, limits::kBackupDailyMax);

    settings_ = std::move(next);

    AppendJournal("update_settings",
        "title=" + settings_.title +
        " hotkey=" + settings_.hotkey +
        " execute_hide=" + (settings_.execute_hide ? "1" : "0") +
        " locked=" + (settings_.locked ? "1" : "0") +
        " auto_hide=" + (settings_.auto_hide ? "1" : "0") +
        " autorun=" + (settings_.autorun ? "1" : "0") +
        " start_hidden=" + (settings_.start_hidden ? "1" : "0") +
        " close_minimize=" + (settings_.close_minimize ? "1" : "0") +
        " double_click=" + (settings_.double_click_launch ? "1" : "0") +
        " backups=" + std::to_string(settings_.backup_rolling_count) + "/" + std::to_string(settings_.backup_daily_days) +
        " panel_width=" + std::to_string(static_cast<int>(settings_.group_panel_width)));
    return SaveSettings(error);
}

std::vector<BackupEntry> LauncherBackend::ListBackups() const {
    std::vector<BackupEntry> out;
    const auto backup_dir = base_dir_ / "backups";
    std::error_code ec;
    if (!std::filesystem::exists(backup_dir, ec)) {
        return out;
    }

    for (const auto& entry : std::filesystem::directory_iterator(backup_dir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto name = entry.path().filename().string();
        const auto kind = ClassifyBackupName(name);
        if (kind.empty()) {
            continue;
        }
        BackupEntry item;
        item.path = entry.path();
        item.name = name;
        item.kind = kind;
        const auto mtime = std::filesystem::last_write_time(entry.path(), ec);
        if (!ec) {
            item.modified_time = std::chrono::duration_cast<std::chrono::seconds>(
                mtime.time_since_epoch()).count();
        }
        item.size = entry.file_size(ec);
        out.push_back(std::move(item));
    }

    std::sort(out.begin(), out.end(), [](const BackupEntry& lhs, const BackupEntry& rhs) {
        return lhs.name > rhs.name;
    });
    return out;
}

bool LauncherBackend::RestoreFromBackup(const std::filesystem::path& backup_path, std::string* error) {
    if (!EnsureLoaded(error)) {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(backup_path, ec)) {
        SetError(error, "backup file not found");
        return false;
    }

    const auto content = ReadTextFile(backup_path);
    bool valid = false;
    auto parsed = ParseJsonText(content, nullptr);
    if (parsed) {
        const auto& root = parsed->root();
        const auto* version = FindField(root, "version");
        const auto* groups = FindField(root, "groups");
        valid = version != nullptr && version->is_number() && version->as_int_or(0) == 2 &&
                groups != nullptr && groups->is_array();
    }
    if (!valid) {
        SetError(error, "backup file is not a valid v2 dataset");
        return false;
    }

    if (!WriteTextAtomic(data_path_, content, error)) {
        return false;
    }

    AppendJournal("restore_backup", "from=" + backup_path.filename().string());
    last_load_corrupted_ = false;
    return Load(error);
}

std::string LauncherBackend::ComputeDataMd5Hex(std::string* error) const {
    if (!std::filesystem::exists(data_path_)) {
        SetError(error, "data file not found");
        return {};
    }
    const auto content = ReadTextFile(data_path_);
    return ComputeMd5Hex(content, error);
}

bool LauncherBackend::ConsumeLastLoadCorrupted() {
    return std::exchange(last_load_corrupted_, false);
}

} // namespace core
