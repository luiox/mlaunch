#include "launcher_core.h"

#include <filesystem>
#include <fstream>
#include <set>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace {

// 测试用假执行器：只记录调用，不真正启动进程。
class FakeLaunchExecutor : public core::LaunchExecutor {
public:
    bool Launch(const std::string& target_path, const std::string& arguments, std::string* error) override {
        launched.emplace_back(target_path, arguments);
        return true;
    }

    std::vector<std::pair<std::string, std::string>> launched;
};

// 测试用假解析器：对 .lnk 返回固定目标。
class FakeShortcutResolver : public core::ShortcutResolver {
public:
    std::optional<std::pair<std::string, std::string>> Resolve(const std::string& shortcut_path) override {
        return std::make_pair(std::string("C:\\resolved\\target.exe"), std::string("/resolved-arg"));
    }
};

std::filesystem::path MakeTempDir(const char* name) {
    const auto root = std::filesystem::temp_directory_path() / "nassistant_cpp_backend_tests" / name;
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    return root;
}

void WriteText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

const core::Group* FindGroupById(const core::LauncherBackend& b, const std::string& id) {
    for (const auto& g : b.Data().groups) {
        if (g.id == id) {
            return &g;
        }
    }
    return nullptr;
}

core::ItemInput MakeItemInput(const std::string& name, const std::string& path) {
    core::ItemInput input;
    input.name = name;
    input.target_path = path;
    input.icon_location = path;
    input.arguments = "";
    return input;
}

std::size_t CountBackups(const std::filesystem::path& base, const std::string& kind) {
    std::size_t count = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(base / "backups", ec)) {
        const auto name = entry.path().filename().string();
        if (name.compare(0, 12, "launcher.v2.") != 0) {
            continue;
        }
        if (name.size() != 12 + 5 + 15 && name.size() != 12 + 5 + 8) {
            continue;
        }
        const auto stamp = name.substr(12, name.size() - 17);
        const bool rolling = stamp.size() == 15 && stamp[8] == '-';
        const bool daily = stamp.size() == 8;
        if (kind == "rolling" && rolling) ++count;
        if (kind == "daily" && daily) ++count;
    }
    return count;
}

TEST(BackendTest, ImportsLegacyDataJson) {
    const auto legacy = MakeTempDir("legacy_import");
    const auto base = MakeTempDir("base_import");

    WriteText(legacy / "Data.json", R"({
        "Common": [
            {
                "Name": "Everything",
                "TargetPath": "C:\\Program Files\\Everything\\Everything.exe",
                "IconLocation": "C:\\Program Files\\Everything\\Everything.exe",
                "Arguments": "",
                "Count": 12
            }
        ]
    })");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;
    ASSERT_FALSE(b.Data().groups.empty());
    EXPECT_EQ(b.Data().groups[0].name, "Common");
    ASSERT_FALSE(b.Data().groups[0].items.empty());
    EXPECT_EQ(b.Data().groups[0].items[0].name, "Everything");
    EXPECT_TRUE(std::filesystem::exists(base / "launcher.v2.json"));
}

TEST(BackendTest, GroupAndItemCrudWorks) {
    const auto legacy = MakeTempDir("legacy_crud");
    const auto base = MakeTempDir("base_crud");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto id = b.AddGroup("Tools", &error);
    ASSERT_FALSE(id.empty()) << error;
    ASSERT_TRUE(b.RenameGroup(id, "MyTools", &error)) << error;

    core::ItemInput input;
    input.name = "Procmon";
    input.target_path = "C:\\Tools\\Procmon.exe";
    input.icon_location = "C:\\Tools\\Procmon.exe";
    input.arguments = "";

    ASSERT_TRUE(b.UpsertItem(id, input, &error)) << error;

    const auto* group = [&]() -> const core::Group* {
        for (const auto& g : b.Data().groups) {
            if (g.id == id) {
                return &g;
            }
        }
        return nullptr;
    }();

    ASSERT_NE(group, nullptr);
    ASSERT_EQ(group->name, "MyTools");
    ASSERT_EQ(group->items.size(), 1u);
    const auto item_id = group->items[0].id;

    ASSERT_TRUE(b.DeleteItem(id, item_id, &error)) << error;

    const auto* group_after = [&]() -> const core::Group* {
        for (const auto& g : b.Data().groups) {
            if (g.id == id) {
                return &g;
            }
        }
        return nullptr;
    }();
    ASSERT_NE(group_after, nullptr);
    EXPECT_TRUE(group_after->items.empty());
}

TEST(BackendTest, MoveItemAcrossGroups) {
    const auto legacy = MakeTempDir("legacy_move");
    const auto base = MakeTempDir("base_move");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto g1 = b.AddGroup("G1", &error);
    const auto g2 = b.AddGroup("G2", &error);
    ASSERT_FALSE(g1.empty());
    ASSERT_FALSE(g2.empty());

    core::ItemInput input;
    input.name = "Notepad";
    input.target_path = "C:\\Windows\\notepad.exe";
    input.icon_location = "C:\\Windows\\notepad.exe";
    input.arguments = "";
    ASSERT_TRUE(b.UpsertItem(g1, input, &error)) << error;

    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == g1 && !g.items.empty()) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_FALSE(item_id.empty());

    ASSERT_TRUE(b.MoveItem(g1, item_id, g2, &error)) << error;

    std::size_t c1 = 0;
    std::size_t c2 = 0;
    for (const auto& g : b.Data().groups) {
        if (g.id == g1) c1 = g.items.size();
        if (g.id == g2) c2 = g.items.size();
    }
    EXPECT_EQ(c1, 0u);
    EXPECT_EQ(c2, 1u);
}

TEST(BackendTest, RecoversFromIncompatibleVersion) {
    const auto legacy = MakeTempDir("legacy_incompatible_version");
    const auto base = MakeTempDir("base_incompatible_version");

    WriteText(base / "launcher.v2.json", R"({
        "version": 999,
        "groups": []
    })");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;
    ASSERT_EQ(b.Data().version, 2);
    ASSERT_FALSE(b.Data().groups.empty());

    bool has_backup = false;
    for (const auto& entry : std::filesystem::directory_iterator(base)) {
        const auto name = entry.path().filename().string();
        if (name.find("launcher.v2.json.bad.") == 0) {
            has_backup = true;
            break;
        }
    }
    EXPECT_TRUE(has_backup);
}

TEST(BackendTest, RecoversFromCorruptedJson) {
    const auto legacy = MakeTempDir("legacy_corrupted_json");
    const auto base = MakeTempDir("base_corrupted_json");

    WriteText(base / "launcher.v2.json", "{ invalid json");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;
    ASSERT_FALSE(b.Data().groups.empty());

    bool has_backup = false;
    for (const auto& entry : std::filesystem::directory_iterator(base)) {
        const auto name = entry.path().filename().string();
        if (name.find("launcher.v2.json.bad.") == 0) {
            has_backup = true;
            break;
        }
    }
    EXPECT_TRUE(has_backup);
}

TEST(BackendTest, BackupRotationKeepsRecentFiveAndDailySnapshot) {
    const auto legacy = MakeTempDir("legacy_backup_rotation");
    const auto base = MakeTempDir("base_backup_rotation");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    for (int i = 0; i < 8; ++i) {
        ASSERT_TRUE(b.SaveData(&error)) << error;
    }

    EXPECT_GT(CountBackups(base, "rolling"), std::size_t{0});
    EXPECT_LE(CountBackups(base, "rolling"), std::size_t{5});
    EXPECT_EQ(CountBackups(base, "daily"), std::size_t{1});
}

TEST(BackendTest, SoftDeleteMovesItemIntoHiddenRecycleBin) {
    const auto legacy = MakeTempDir("legacy_soft_delete");
    const auto base = MakeTempDir("base_soft_delete");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_FALSE(gid.empty()) << error;
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Procmon", "C:\\Tools\\Procmon.exe"), &error)) << error;

    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && !g.items.empty()) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_FALSE(item_id.empty());

    ASSERT_TRUE(b.DeleteItem(gid, item_id, &error)) << error;

    const auto* source = FindGroupById(b, gid);
    ASSERT_NE(source, nullptr);
    EXPECT_TRUE(source->items.empty());

    const auto* bin = FindGroupById(b, core::kRecycleBinGroupId);
    ASSERT_NE(bin, nullptr);
    EXPECT_TRUE(bin->hidden);
    ASSERT_EQ(bin->items.size(), 1u);
    EXPECT_EQ(bin->items[0].id, item_id);
}

TEST(BackendTest, UndoRestoresDeletedItemToOriginalPlace) {
    const auto legacy = MakeTempDir("legacy_undo");
    const auto base = MakeTempDir("base_undo");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("First", "C:\\Tools\\1.exe"), &error));
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Second", "C:\\Tools\\2.exe"), &error));

    std::string first_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && g.items.size() == 2) {
            first_id = g.items[0].id;
        }
    }
    ASSERT_FALSE(first_id.empty());

    ASSERT_TRUE(b.DeleteItem(gid, first_id, &error)) << error;
    ASSERT_TRUE(b.UndoLastDelete(&error)) << error;

    const auto* group = FindGroupById(b, gid);
    ASSERT_NE(group, nullptr);
    ASSERT_EQ(group->items.size(), 2u);
    EXPECT_EQ(group->items[0].id, first_id);

    const auto* bin = FindGroupById(b, core::kRecycleBinGroupId);
    ASSERT_NE(bin, nullptr);
    EXPECT_TRUE(bin->items.empty());
}

TEST(BackendTest, DeleteInsideRecycleBinPurgesPermanently) {
    const auto legacy = MakeTempDir("legacy_purge");
    const auto base = MakeTempDir("base_purge");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Procmon", "C:\\Tools\\Procmon.exe"), &error));

    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && !g.items.empty()) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_TRUE(b.DeleteItem(gid, item_id, &error)) << error;
    ASSERT_TRUE(b.DeleteItem(core::kRecycleBinGroupId, item_id, &error)) << error;

    const auto* bin = FindGroupById(b, core::kRecycleBinGroupId);
    ASSERT_NE(bin, nullptr);
    EXPECT_TRUE(bin->items.empty());

    std::string undo_error;
    EXPECT_FALSE(b.UndoLastDelete(&undo_error));
}

TEST(BackendTest, RecycleBinIsProtectedFromDirectModification) {
    const auto legacy = MakeTempDir("legacy_bin_guard");
    const auto base = MakeTempDir("base_bin_guard");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Procmon", "C:\\Tools\\Procmon.exe"), &error));
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Notepad", "C:\\Tools\\Notepad.exe"), &error));

    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && g.items.size() == 2) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_TRUE(b.DeleteItem(gid, item_id, &error)) << error;

    std::string remaining_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && g.items.size() == 1) {
            remaining_id = g.items[0].id;
        }
    }
    ASSERT_FALSE(remaining_id.empty());

    std::string guard_error;
    EXPECT_FALSE(b.DeleteGroup(core::kRecycleBinGroupId, gid, &guard_error));
    EXPECT_FALSE(b.MoveItem(gid, remaining_id, core::kRecycleBinGroupId, &guard_error));
    EXPECT_FALSE(b.RenameGroup(core::kRecycleBinGroupId, "NotBin", &guard_error));
    EXPECT_FALSE(b.UpsertItem(core::kRecycleBinGroupId, MakeItemInput("X", "C:\\x.exe"), &guard_error));
    EXPECT_FALSE(b.ReorderGroup(core::kRecycleBinGroupId, 0, &guard_error));
}

TEST(BackendTest, RestoreFromBackupRecoversDataset) {
    const auto legacy = MakeTempDir("legacy_restore");
    const auto base = MakeTempDir("base_restore");

    core::LauncherBackend b1(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b1.Load(&error)) << error;

    const auto gid = b1.AddGroup("Tools", &error);
    ASSERT_FALSE(gid.empty()) << error;
    ASSERT_TRUE(b1.RenameGroup(gid, "MyTools", &error)) << error;
    // Rotation snapshots the pre-save state, so persist once more to make the
    // renamed state available in backups.
    ASSERT_TRUE(b1.SaveData(&error)) << error;

    WriteText(base / "launcher.v2.json", "{ corrupted now");

    core::LauncherBackend b2(base, legacy, nullptr, nullptr);
    ASSERT_TRUE(b2.Load(&error)) << error;
    EXPECT_TRUE(b2.ConsumeLastLoadCorrupted());
    EXPECT_FALSE(b2.ConsumeLastLoadCorrupted());

    const auto backups = b2.ListBackups();
    ASSERT_FALSE(backups.empty());

    ASSERT_TRUE(b2.RestoreFromBackup(backups.front().path, &error)) << error;
    const auto* group = FindGroupById(b2, gid);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->name, "MyTools");
}

TEST(BackendTest, ComputeDataMd5HexReturnsStableHash) {
    const auto legacy = MakeTempDir("legacy_md5");
    const auto base = MakeTempDir("base_md5");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto hash1 = b.ComputeDataMd5Hex(&error);
    ASSERT_EQ(hash1.size(), 32u) << error;
    const auto hash2 = b.ComputeDataMd5Hex(&error);
    EXPECT_EQ(hash1, hash2);

    for (char ch : hash1) {
        const bool hex = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
        ASSERT_TRUE(hex) << "non-hex char: " << ch;
    }

    const auto gid = b.AddGroup("Another", &error);
    ASSERT_FALSE(gid.empty());
    const auto hash3 = b.ComputeDataMd5Hex(&error);
    EXPECT_NE(hash1, hash3);
}

TEST(BackendTest, JournalRecordsMutatingOperations) {
    const auto legacy = MakeTempDir("legacy_journal");
    const auto base = MakeTempDir("base_journal");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_FALSE(gid.empty()) << error;
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Procmon", "C:\\Tools\\Procmon.exe"), &error)) << error;

    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && !g.items.empty()) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_TRUE(b.DeleteItem(gid, item_id, &error)) << error;

    std::ifstream in(base / "operations.log");
    ASSERT_TRUE(in.good());
    std::stringstream ss;
    ss << in.rdbuf();
    const auto content = ss.str();
    EXPECT_NE(content.find("add_group"), std::string::npos);
    EXPECT_NE(content.find("add_item"), std::string::npos);
    EXPECT_NE(content.find("delete_item"), std::string::npos);
}

TEST(BackendTest, HiddenGroupsExcludedFromNameConflicts) {
    const auto legacy = MakeTempDir("legacy_hidden_conflict");
    const auto base = MakeTempDir("base_hidden_conflict");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Procmon", "C:\\Tools\\Procmon.exe"), &error));
    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && !g.items.empty()) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_TRUE(b.DeleteItem(gid, item_id, &error)) << error;

    // The hidden recycle bin is named "Recycle Bin"; creating a visible group
    // with the same name must still succeed.
    const auto dup = b.AddGroup(core::kRecycleBinGroupName, &error);
    EXPECT_FALSE(dup.empty()) << error;
}

} // namespace

TEST(BackendTest, LaunchUsesInjectedExecutor) {
    const auto legacy = MakeTempDir("legacy_launch_fake");
    const auto base = MakeTempDir("base_launch_fake");

    FakeLaunchExecutor executor;
    core::LauncherBackend b(base, legacy, &executor, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_FALSE(gid.empty()) << error;
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Procmon", "C:\\Tools\\Procmon.exe"), &error)) << error;

    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && !g.items.empty()) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_FALSE(item_id.empty());

    const auto result = b.Launch(gid, item_id, &error);
    ASSERT_TRUE(result.ok) << error;
    ASSERT_EQ(executor.launched.size(), 1u);
    EXPECT_EQ(executor.launched[0].first, "C:\\Tools\\Procmon.exe");

    const auto* group = FindGroupById(b, gid);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->items[0].launch_count, 1u);
}

TEST(BackendTest, DropImportUsesInjectedResolver) {
    const auto legacy = MakeTempDir("legacy_drop_fake");
    const auto base = MakeTempDir("base_drop_fake");

    FakeShortcutResolver resolver;
    core::LauncherBackend b(base, legacy, nullptr, &resolver);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_FALSE(gid.empty()) << error;

    const auto created = b.CreateItemsFromDroppedPaths(gid, {"C:\\drop\\app.lnk"}, &error);
    ASSERT_EQ(created, 1u) << error;

    const auto* group = FindGroupById(b, gid);
    ASSERT_NE(group, nullptr);
    ASSERT_EQ(group->items.size(), 1u);
    EXPECT_EQ(group->items[0].target_path, "C:\\resolved\\target.exe");
    EXPECT_EQ(group->items[0].arguments, "/resolved-arg");
}

TEST(BackendTest, ImportPonerDataIsIdempotent) {
    const auto legacy = MakeTempDir("legacy_poner_import");
    const auto base = MakeTempDir("base_poner_import");

    WriteText(legacy / "Data.json", R"({
        "Common": [
            {"Name": "Everything", "TargetPath": "C:\\Tools\\Everything.exe", "IconLocation": "C:\\Tools\\Everything.exe", "Arguments": "", "Count": 5},
            {"Name": "----tools----", "TargetPath": "", "IconLocation": "", "Arguments": "", "Count": 0}
        ]
    })");

    FakeLaunchExecutor executor;
    core::LauncherBackend b(base, legacy, &executor, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto first = b.ImportPonerData(legacy / "Data.json", &error);
    ASSERT_GT(first, 0u) << error;
    const auto second = b.ImportPonerData(legacy / "Data.json", &error);
    ASSERT_GT(second, 0u) << error;

    const auto* group = FindGroupById(b, [&] {
        for (const auto& g : b.Data().groups) {
            if (g.name == "Common") return g.id;
        }
        return std::string();
    }());
    ASSERT_NE(group, nullptr);

    std::size_t app_items = 0;
    std::size_t separators = 0;
    for (const auto& item : group->items) {
        if (item.item_type == "separator") ++separators; else ++app_items;
    }
    EXPECT_EQ(app_items, 1u) << "duplicate import must not append";
    EXPECT_EQ(separators, 1u) << "separator must be deduped by name";
    EXPECT_EQ(group->items[0].launch_count, 5u);
}

TEST(BackendTest, ImportPonerDataUpdatesCountAndAppendsNew) {
    const auto legacy = MakeTempDir("legacy_poner_update");
    const auto base = MakeTempDir("base_poner_update");

    WriteText(legacy / "Data.json", R"({
        "Common": [
            {"Name": "Everything", "TargetPath": "C:\\Tools\\Everything.exe", "IconLocation": "C:\\Tools\\Everything.exe", "Arguments": "", "Count": 5}
        ]
    })");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;
    ASSERT_GT(b.ImportPonerData(legacy / "Data.json", &error), 0u) << error;

    // Poner 侧继续日用：Count 变化 + 新条目出现，再次导入应合并而非重复。
    WriteText(legacy / "Data.json", R"({
        "Common": [
            {"Name": "Everything", "TargetPath": "C:\\Tools\\Everything.exe", "IconLocation": "C:\\Tools\\Everything.exe", "Arguments": "", "Count": 9},
            {"Name": "Procmon", "TargetPath": "C:\\Tools\\Procmon.exe", "IconLocation": "C:\\Tools\\Procmon.exe", "Arguments": "", "Count": 1}
        ],
        "IDE": [
            {"Name": "VSCode", "TargetPath": "C:\\Tools\\vscode.exe", "IconLocation": "C:\\Tools\\vscode.exe", "Arguments": "", "Count": 3}
        ]
    })");

    const auto merged = b.ImportPonerData(legacy / "Data.json", &error);
    ASSERT_GT(merged, 0u) << error;

    const auto* common = FindGroupById(b, [&] {
        for (const auto& g : b.Data().groups) {
            if (g.name == "Common") return g.id;
        }
        return std::string();
    }());
    ASSERT_NE(common, nullptr);
    ASSERT_EQ(common->items.size(), 2u);
    EXPECT_EQ(common->items[0].launch_count, 9u);
    EXPECT_EQ(common->items[0].name, "Everything");

    const auto* ide = FindGroupById(b, [&] {
        for (const auto& g : b.Data().groups) {
            if (g.name == "IDE") return g.id;
        }
        return std::string();
    }());
    ASSERT_NE(ide, nullptr) << "missing group must be created";
    ASSERT_EQ(ide->items.size(), 1u);
    EXPECT_EQ(ide->items[0].name, "VSCode");

    // 导入前快照：backups/ 至少有一份 rolling 备份。
    std::size_t backups = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(base / "backups", ec)) {
        if (entry.is_regular_file()) ++backups;
    }
    EXPECT_GT(backups, 0u);
}

TEST(BackendTest, LaunchExpandsPlaceholdersAndEnvVars) {
    const auto legacy = MakeTempDir("legacy_placeholder");
    const auto base = MakeTempDir("base_placeholder");

    FakeLaunchExecutor executor;
    core::LauncherBackend b(base, legacy, &executor, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;
    b.SetAppDir("D:\\Apps\\mlaunch");

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_FALSE(gid.empty()) << error;

    core::ItemInput input;
    input.name = "Tool";
    input.target_path = "%pr%\\tools\\tool.exe";
    input.icon_location = "%pr%\\tools\\tool.exe";
    input.arguments = "--root %cr%data --user %USERNAME%";
    ASSERT_TRUE(b.UpsertItem(gid, input, &error)) << error;

    std::string item_id;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid && !g.items.empty()) {
            item_id = g.items[0].id;
        }
    }
    ASSERT_FALSE(item_id.empty());

    const auto result = b.Launch(gid, item_id, &error);
    ASSERT_TRUE(result.ok) << error;
    ASSERT_EQ(executor.launched.size(), 1u);
    EXPECT_EQ(executor.launched[0].first, "D:\\Apps\\mlaunch\\tools\\tool.exe");
    EXPECT_NE(executor.launched[0].second.find("--root D:\\data"), std::string::npos);
    EXPECT_NE(executor.launched[0].second.find("--user "), std::string::npos);
    // %USERNAME% 应被展开为非空值（不再包含百分号）
    const auto& args = executor.launched[0].second;
    const auto user_pos = args.find("--user ");
    EXPECT_EQ(args.find('%', user_pos), std::string::npos) << args;
}

TEST(BackendTest, SortGroupItemsByNameIsCaseInsensitiveAndJournaled) {
    const auto legacy = MakeTempDir("legacy_sort");
    const auto base = MakeTempDir("base_sort");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Tools", &error);
    ASSERT_FALSE(gid.empty()) << error;
    const char* names[] = {"banana", "Apple", "cherry", "apricot"};
    for (const auto* name : names) {
        ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput(name, "C:\\x.exe"), &error)) << error;
    }

    EXPECT_TRUE(b.SortGroupItemsByName(gid, &error)) << error;

    // 回收站不允许排序。
    std::string bin_error;
    EXPECT_FALSE(b.SortGroupItemsByName(core::kRecycleBinGroupId, &bin_error));

    const core::Group* group = nullptr;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid) group = &g;
    }
    ASSERT_NE(group, nullptr);
    ASSERT_EQ(group->items.size(), 4u);
    EXPECT_EQ(group->items[0].name, "Apple");
    EXPECT_EQ(group->items[1].name, "apricot");
    EXPECT_EQ(group->items[2].name, "banana");
    EXPECT_EQ(group->items[3].name, "cherry");

    // journal 记录 sort_group 动作。
    std::ifstream journal(base / "operations.log");
    std::string content((std::istreambuf_iterator<char>(journal)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("sort_group"), std::string::npos);
}

TEST(BackendTest, ExportDataWritesStandaloneSnapshot) {
    const auto legacy = MakeTempDir("legacy_export");
    const auto base = MakeTempDir("base_export");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Exported", &error);
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("Tool", "C:\\tool.exe"), &error)) << error;

    const auto export_dir = MakeTempDir("export_out");
    const auto target = export_dir / L"sub" / L"export.json";
    EXPECT_TRUE(b.ExportData(target, &error)) << error;

    // 导出文件可被全新 backend 完整读回。
    const auto import_base = MakeTempDir("base_export_roundtrip");
    WriteText(import_base / "launcher.v2.json", [&] {
        std::ifstream in(target, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }());
    core::LauncherBackend b2(import_base, import_base / "nope", nullptr, nullptr);
    std::string error2;
    ASSERT_TRUE(b2.Load(&error2)) << error2;
    bool found = false;
    for (const auto& g : b2.Data().groups) {
        for (const auto& i : g.items) {
            if (g.name == "Exported" && i.name == "Tool") found = true;
        }
    }
    EXPECT_TRUE(found);

    // 空路径拒绝导出。
    std::string empty_error;
    EXPECT_FALSE(b.ExportData(std::filesystem::path(), &empty_error));
}

TEST(BackendTest, UpdateSettingsPersistsAndClamps) {
    const auto legacy = MakeTempDir("legacy_settings");
    const auto base = MakeTempDir("base_settings");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    core::Settings next = b.CurrentSettings();
    next.hotkey = "  Ctrl+Alt+Space ";
    next.execute_hide = false;
    next.group_panel_width = 9999.0;   // 应钳到 600
    next.main_window_width = 1200.0;
    next.main_window_height = 800.0;
    EXPECT_TRUE(b.UpdateSettings(next, &error)) << error;

    EXPECT_EQ(b.CurrentSettings().hotkey, "Ctrl+Alt+Space");
    EXPECT_FALSE(b.CurrentSettings().execute_hide);
    EXPECT_DOUBLE_EQ(b.CurrentSettings().group_panel_width, 600.0);

    // 新实例重新加载后设置仍在（持久化验证）。
    core::LauncherBackend b2(base, legacy, nullptr, nullptr);
    std::string error2;
    ASSERT_TRUE(b2.Load(&error2)) << error2;
    EXPECT_EQ(b2.CurrentSettings().hotkey, "Ctrl+Alt+Space");
    EXPECT_FALSE(b2.CurrentSettings().execute_hide);
    EXPECT_DOUBLE_EQ(b2.CurrentSettings().main_window_width, 1200.0);
}

TEST(BackendTest, ClearGroupSoftDeletesAllAndUndoRestoresLast) {
    const auto legacy = MakeTempDir("legacy_clear");
    const auto base = MakeTempDir("base_clear");

    core::LauncherBackend b(base, legacy, nullptr, nullptr);
    std::string error;
    ASSERT_TRUE(b.Load(&error)) << error;

    const auto gid = b.AddGroup("Wipe", &error);
    ASSERT_FALSE(gid.empty()) << error;
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("one", "C:\\1.exe"), &error)) << error;
    ASSERT_TRUE(b.UpsertItem(gid, MakeItemInput("two", "C:\\2.exe"), &error)) << error;

    EXPECT_EQ(b.ClearGroup(gid, &error), 2u) << error;

    // 分组清空，回收站 +2。
    const core::Group* group = nullptr;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid) group = &g;
    }
    ASSERT_NE(group, nullptr);
    EXPECT_TRUE(group->items.empty());

    // 回收站不允许清空。
    std::string bin_error;
    EXPECT_EQ(b.ClearGroup(core::kRecycleBinGroupId, &bin_error), 0u);

    // 空分组再清空报错。
    std::string empty_error;
    EXPECT_EQ(b.ClearGroup(gid, &empty_error), 0u);

    // Ctrl+Z 恢复最后删除的一条。
    EXPECT_TRUE(b.UndoLastDelete(&error)) << error;
    for (const auto& g : b.Data().groups) {
        if (g.id == gid) group = &g;
    }
    ASSERT_NE(group, nullptr);
    ASSERT_EQ(group->items.size(), 1u);
    EXPECT_EQ(group->items[0].name, "two");

    // journal 记录 clear_group。
    std::ifstream journal(base / "operations.log");
    std::string content((std::istreambuf_iterator<char>(journal)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("clear_group"), std::string::npos);
}
