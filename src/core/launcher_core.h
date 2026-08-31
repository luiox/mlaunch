#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace core {

/** @brief Fixed id of the built-in recycle bin group (hidden from normal flow). */
constexpr char kRecycleBinGroupId[] = "recycle_bin";
/** @brief Display name of the built-in recycle bin group. */
constexpr char kRecycleBinGroupName[] = "Recycle Bin";

/** @brief Single launchable entry in a group. */
struct LaunchItem {
    std::string id;
    std::string item_type;
    std::string name;
    std::string target_path;
    std::string icon_location;
    std::string arguments;
    /// 工作目录（起始位置）；空 = 不指定（沿用目标所在目录的 ShellExecute 默认）。
    std::string working_dir;
    std::uint64_t launch_count = 0;
    bool enabled = true;
};

/** @brief A launcher group containing ordered launch items. */
struct Group {
    std::string id;
    std::string name;
    int order = 0;
    bool hidden = false;
    std::vector<LaunchItem> items;
};

/** @brief Persistent launcher dataset stored in launcher.v2.json. */
struct LauncherData {
    int version = 2;
    std::vector<Group> groups;
};

/** @brief Runtime settings loaded from nassistant.settings.json. */
struct Settings {
    std::string hotkey = "Alt+1";
    bool execute_hide = true;
    /// 锁定布局：禁用窗口拖动/缩放、分隔条与列表拖拽重排。
    bool locked = false;
    /// 失焦自动隐藏：窗口失去激活（且非自家弹窗获焦）时隐藏，热键唤回。
    bool auto_hide = false;
    /// 开机自启：HKCU Run 键注册表项的镜像（注册表写放在 UI 层完成）。
    bool autorun = false;
    std::optional<std::string> current_group;
    double group_panel_width = 220.0;
    double main_window_width = 1040.0;
    double main_window_height = 700.0;
};

/** @brief Input payload used for add/edit item operations. */
struct ItemInput {
    std::optional<std::string> id;
    std::optional<std::string> item_type;
    std::string name;
    std::string target_path;
    std::string icon_location;
    std::string arguments;
    std::string working_dir;
    std::optional<bool> enabled;
};

    /** @brief Result returned by Launch operation. */
    struct LaunchResult {
        bool ok = false;
        std::string message;
    };

    /** @brief A backup file discovered under backups/. */
    struct BackupEntry {
        std::filesystem::path path;
        std::string name;
        std::string kind; // "rolling" or "daily"
        std::int64_t modified_time = 0;
        std::uintmax_t size = 0;
    };

/**
 * @brief Abstraction over process launching, injected so core stays shell-free.
 */
class LaunchExecutor {
public:
    virtual ~LaunchExecutor() = default;
    virtual bool Launch(const std::string& target_path, const std::string& arguments,
                         const std::string& working_dir, std::string* error) = 0;
};

/**
 * @brief Abstraction over .lnk shortcut resolution, injected so core stays shell-free.
 */
class ShortcutResolver {
public:
    virtual ~ShortcutResolver() = default;
    virtual std::optional<std::pair<std::string, std::string>> Resolve(const std::string& shortcut_path) = 0;
};

/**
 * @brief Launcher data service handling persistence and core CRUD behaviors.
 */
class LauncherBackend {
public:
    /// @param launch_executor required for Launch(); @param shortcut_resolver required for .lnk drops.
    LauncherBackend(std::filesystem::path base_dir,
                    std::filesystem::path legacy_root,
                    LaunchExecutor* launch_executor,
                    ShortcutResolver* shortcut_resolver);

    /** @brief Load data and settings; recovers from incompatible/corrupted JSON when possible. */
    bool Load(std::string* error = nullptr);
    /** @brief Save launcher dataset to launcher.v2.json. */
    bool SaveData(std::string* error = nullptr) const;
    /** @brief Save settings to nassistant.settings.json. */
    bool SaveSettings(std::string* error = nullptr) const;

    /**
     * @brief Add a new group.
     * @param name Group name (trimmed).
     * @param error Optional output error message.
     * @return New group id, or empty string on failure.
     */
    std::string AddGroup(const std::string& name, std::string* error = nullptr);
    bool RenameGroup(const std::string& group_id, const std::string& name, std::string* error = nullptr);
    bool DeleteGroup(const std::string& group_id, const std::string& target_group_id, std::string* error = nullptr);
    bool UpsertItem(const std::string& group_id, const ItemInput& input, std::string* error = nullptr);
    bool DeleteItem(const std::string& group_id, const std::string& item_id, std::string* error = nullptr);
    bool MoveItem(const std::string& group_id, const std::string& item_id, const std::string& target_group_id, std::string* error = nullptr);
    bool ReorderGroup(const std::string& group_id, int target_index, std::string* error = nullptr);
    bool ReorderItemInGroup(const std::string& group_id, const std::string& item_id, int target_index, std::string* error = nullptr);
    LaunchResult Launch(const std::string& group_id, const std::string& item_id, std::string* error = nullptr);
    std::size_t CreateItemsFromDroppedPaths(const std::string& group_id, const std::vector<std::string>& paths, std::string* error = nullptr);

    /** @brief Undo the most recent soft delete; returns the item to its original group and index. */
    bool UndoLastDelete(std::string* error = nullptr);
    /**
     * @brief Idempotent import from legacy Poner Data.json.
     *
     * Merge key is (group name, target path): existing items get Count/name
     * refreshed, new items are appended, missing groups are created. Safe to
     * re-run while Poner is still in daily use.
     * @return Number of items merged (updated + appended).
     */
    std::size_t ImportPonerData(const std::filesystem::path& legacy_json_path, std::string* error = nullptr);
    /** @brief Sort one group's items by display name (case-insensitive, stable). */
    bool SortGroupItemsByName(const std::string& group_id, std::string* error = nullptr);
    /** @brief Write the current dataset to an arbitrary path without touching backups. */
    bool ExportData(const std::filesystem::path& target_path, std::string* error = nullptr);
    /** @brief Validate, apply and persist new settings (clamps sizes, trims hotkey). */
    bool UpdateSettings(const Settings& settings, std::string* error = nullptr);
    /**
     * @brief 在绝对路径与 %pr%/%cr% 占位符形式之间转换条目路径（便携模式）。
     *
     * 与 Launch 的占位符语义一致：%pr% = 程序目录，%cr% = 程序所在盘根。
     * 转换范围为全部分组的条目 target_path 与 icon_location；分隔条目、
     * 已是目标形式的路径、不同盘的绝对路径保持不变（幂等可重跑）。
     * @param to_relative true=绝对→占位符；false=占位符→绝对。
     * @return 发生变化的条目数；-1 表示失败（error 给出原因）。
     */
    int ConvertItemPaths(bool to_relative, std::string* error = nullptr);
    /** @brief Directory treated as the legacy Poner install root. */
    const std::filesystem::path& LegacyRoot() const { return legacy_root_; }
    /** @brief App install directory used to expand %pr%/%cr% placeholders at launch. */
    void SetAppDir(std::filesystem::path dir);
    /** @brief List backup files under backups/, newest first. */
    std::vector<BackupEntry> ListBackups() const;
    /** @brief Validate a backup file and restore it as the current dataset. */
    bool RestoreFromBackup(const std::filesystem::path& backup_path, std::string* error = nullptr);
    /** @brief MD5 hex of the current data file content (used by destructive-operation confirm). */
    std::string ComputeDataMd5Hex(std::string* error = nullptr) const;
    /** @brief Returns true once if the last Load detected corruption/version mismatch. */
    bool ConsumeLastLoadCorrupted();
    /** @brief True when group_id is the built-in recycle bin id. */
    static bool IsRecycleBinId(const std::string& group_id);
    /** @brief True when the given group is marked hidden. */
    bool IsGroupHidden(const std::string& group_id) const;

    const LauncherData& Data() const { return data_; }
    const Settings& CurrentSettings() const { return settings_; }
    const std::filesystem::path& DataPath() const { return data_path_; }

private:
    static std::string Trim(const std::string& value);
    static std::string ToLowerAscii(std::string value);
    static bool IsSeparatorItem(const std::string& name, const std::string& target, const std::string& icon);
    static std::vector<std::string> SplitWindowsArgs(const std::string& arguments);
    static std::string BasenameNoExt(const std::string& path);
    static std::string NormalizeDroppedPath(const std::string& raw_path);
    static std::string PercentDecodePath(const std::string& input);

    std::string GenerateId(const std::string& prefix);
    LauncherData DefaultLauncherData() const;
    Settings ParsePonerCfg(const std::filesystem::path& cfg_path) const;
    std::string SerializeCurrentData() const;
    bool EnsureLoaded(std::string* error) const;
    Group* FindGroup(const std::string& group_id);
    const Group* FindGroup(const std::string& group_id) const;
    Group* EnsureRecycleBin();
    void RotateBackupsBeforeSave() const;
    void AppendJournal(const std::string& action, const std::string& detail) const;

    struct DeletedItemSnapshot {
        LaunchItem item;
        std::string from_group_id;
        std::string from_group_name;
        std::size_t index = 0;
    };

    std::filesystem::path base_dir_;
    std::filesystem::path legacy_root_;
    std::filesystem::path data_path_;
    std::filesystem::path settings_path_;

    LaunchExecutor* launch_executor_ = nullptr;
    ShortcutResolver* shortcut_resolver_ = nullptr;
    std::filesystem::path app_dir_;

    LauncherData data_;
    Settings settings_;
    bool loaded_ = false;
    bool last_load_corrupted_ = false;
    bool has_last_deleted_ = false;
    DeletedItemSnapshot last_deleted_;

    std::uint64_t id_counter_ = 1;
};

} // namespace core
