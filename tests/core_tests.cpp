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
