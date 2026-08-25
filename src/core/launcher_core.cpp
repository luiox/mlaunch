#include "launcher_core.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <vector>

#include <wincrypt.h>

#include "libca/json/json.hpp"

#include "logger.h"
#include "utils/string_util.h"

namespace core {

namespace {

// ---- libca_json 访问辅助：字段缺失/类型不符时回退默认值 ----

std::string ToStdString(const ca::str::Utf8StringRef& ref) {
    return std::string(reinterpret_cast<const char*>(ref.data()), ref.byte_length());
}

void ReplaceAllInPlace(std::string* text, const std::string& from, const std::string& to) {
    if (from.empty() || text == nullptr) {
        return;
    }
    std::size_t pos = 0;
    while ((pos = text->find(from, pos)) != std::string::npos) {
        text->replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string ExpandEnvUtf8(const std::string& text) {
    std::wstring wide = launcher::util::Utf8ToWide(text);
    wchar_t buffer[32768]{};
    const DWORD size = ::ExpandEnvironmentStringsW(wide.c_str(), buffer, 32768);
    if (size == 0 || size > 32768) {
        return text;
    }
    return launcher::util::WideToUtf8(buffer);
}

const ca::json::JsonValue* FindField(const ca::json::JsonValue& object, const char* key) {
    return object.find(ca::str::Utf8StringRef::from_cstr(key));
}

std::string GetStr(const ca::json::JsonValue& object, const char* key, std::string fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_string()) {
        return ToStdString(v->as_string());
    }
    return fallback;
}

std::uint64_t GetU64(const ca::json::JsonValue& object, const char* key, std::uint64_t fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_number()) {
        return static_cast<std::uint64_t>(v->as_int_or(static_cast<ca::i64>(fallback)));
    }
    return fallback;
}

int GetInt(const ca::json::JsonValue& object, const char* key, int fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_number()) {
        return static_cast<int>(v->as_int_or(fallback));
    }
    return fallback;
}

bool GetBool(const ca::json::JsonValue& object, const char* key, bool fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_bool()) {
        return v->as_bool();
    }
    return fallback;
}

double GetF64(const ca::json::JsonValue& object, const char* key, double fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_number()) {
        return v->as_float_or(fallback);
    }
    return fallback;
}

// 解析 JSON 文本；失败时返回 nullopt 并给出错误描述。
std::optional<ca::json::JsonDocument> ParseJsonText(const std::string& text, std::string* parse_error) {
    auto result = ca::json::JsonReader::read(ca::str::Utf8StringRef::from_string_view(text));
    if (result.is_err()) {
        if (parse_error != nullptr) {
            auto err = std::move(result).unwrap_err();
            *parse_error = ToStdString(err.message.ref());
        }
        return std::nullopt;
    }
    return std::move(result).unwrap();
}

// 组装对象字段的便捷入口：key intern 进 arena，值 move 进 DOM。
void SetStr(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, const std::string& value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_string(arena.intern(value.c_str())));
}

void SetInt(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, std::int64_t value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_int(value));
}

void SetBool(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, bool value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_bool(value));
}

void SetF64(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, double value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_float(value));
}

std::string SerializeDocument(const ca::json::JsonDocument& document) {
    ca::json::JsonWriterOptions options;
    options.pretty = true;
    options.indent = 2;
    const auto text = ca::json::JsonWriter::write(document, options);
    return std::string(reinterpret_cast<const char*>(text.data()), text.byte_length());
}

struct LegacyItem {
    std::string name;
    std::string target_path;
    std::string icon_location;
    std::string arguments;
    std::uint64_t count = 0;
};

void SetError(std::string* out, const std::string& msg) {
    if (out) {
        *out = msg;
    }
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool WriteTextAtomic(const std::filesystem::path& path, const std::string& content, std::string* error) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    const auto tmp = path.string() + ".tmp";

    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            SetError(error, "open temp file failed: " + tmp);
            return false;
        }
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!out.good()) {
            SetError(error, "write temp file failed: " + tmp);
            return false;
        }
    }

    std::filesystem::rename(tmp, path, ec);
    if (!ec) {
        return true;
    }

    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        SetError(error, "replace file failed: " + path.string());
        return false;
    }
    return true;
}

bool BackupCorruptedJson(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }

    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto backup_path = path.string() + ".bad." + std::to_string(timestamp) + ".bak";

    std::error_code ec;
    std::filesystem::copy_file(path, backup_path, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        launcher::log::Warn("backup failed for corrupted json: " + path.string() + " error=" + ec.message());
        return false;
    }

    launcher::log::Warn("backup corrupted json: " + path.string() + " -> " + backup_path);
    return true;
}

std::tm ToLocalTime(std::chrono::system_clock::time_point tp) {
    const auto time_t_value = std::chrono::system_clock::to_time_t(tp);
    std::tm local{};
    localtime_s(&local, &time_t_value);
    return local;
}

std::string FormatJournalTimestamp(std::chrono::system_clock::time_point tp) {
    const auto local = ToLocalTime(tp);
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        local.tm_hour, local.tm_min, local.tm_sec);
    return buffer;
}

std::string FormatFileStamp(std::chrono::system_clock::time_point tp) {
    const auto local = ToLocalTime(tp);
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d-%02d%02d%02d",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        local.tm_hour, local.tm_min, local.tm_sec);
    return buffer;
}

std::string FormatDateStamp(std::chrono::system_clock::time_point tp) {
    const auto local = ToLocalTime(tp);
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    return buffer;
}

constexpr std::size_t kRollingStampLength = 15; // YYYYMMDD-HHMMSS
constexpr std::size_t kDailyStampLength = 8;    // YYYYMMDD

// Returns "rolling", "daily" or "" for non-backup files.
std::string ClassifyBackupName(const std::string& file_name) {
    constexpr const char* kPrefix = "launcher.v2.";
    constexpr const char* kSuffix = ".json";
    const auto prefix_length = std::strlen(kPrefix);
    const auto suffix_length = std::strlen(kSuffix);
    if (file_name.size() <= prefix_length + suffix_length) {
        return {};
    }
    if (file_name.compare(0, prefix_length, kPrefix) != 0) {
        return {};
    }
    if (file_name.compare(file_name.size() - suffix_length, suffix_length, kSuffix) != 0) {
        return {};
    }
    const auto stamp = file_name.substr(prefix_length, file_name.size() - prefix_length - suffix_length);
    if (stamp.size() == kRollingStampLength && stamp[8] == '-') {
        return "rolling";
    }
    if (stamp.size() == kDailyStampLength) {
        return "daily";
    }
    return {};
}

void PruneBackups(const std::filesystem::path& backup_dir) {
    constexpr std::size_t kKeepRolling = 5;
    constexpr std::size_t kKeepDaily = 30;

    std::vector<std::string> rolling;
    std::vector<std::string> daily;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(backup_dir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto name = entry.path().filename().string();
        const auto kind = ClassifyBackupName(name);
        if (kind == "rolling") {
            rolling.push_back(name);
        } else if (kind == "daily") {
            daily.push_back(name);
        }
    }

    // Stamp names sort lexicographically == chronologically; newest first.
    std::sort(rolling.begin(), rolling.end(), std::greater<std::string>());
    std::sort(daily.begin(), daily.end(), std::greater<std::string>());

    for (std::size_t i = kKeepRolling; i < rolling.size(); ++i) {
        std::filesystem::remove(backup_dir / rolling[i], ec);
    }
    for (std::size_t i = kKeepDaily; i < daily.size(); ++i) {
        std::filesystem::remove(backup_dir / daily[i], ec);
    }
}

std::string ToHexLower(const unsigned char* data, std::size_t size) {
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i) {
        out.push_back(kDigits[data[i] >> 4]);
        out.push_back(kDigits[data[i] & 0x0F]);
    }
    return out;
}

std::string ComputeMd5Hex(const std::string& content, std::string* error) {
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    std::string failure;

    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        failure = "CryptAcquireContext failed";
    } else if (!CryptCreateHash(provider, CALG_MD5, 0, 0, &hash)) {
        failure = "CryptCreateHash failed";
    } else {
        const auto* bytes = reinterpret_cast<const BYTE*>(content.data());
        if (!CryptHashData(hash, bytes, static_cast<DWORD>(content.size()), 0)) {
            failure = "CryptHashData failed";
        }
    }

    std::string hex;
    if (failure.empty()) {
        DWORD hash_size = 0;
        DWORD value_size = sizeof(hash_size);
        unsigned char digest[16]{};
        DWORD digest_size = sizeof(digest);
        if (CryptGetHashParam(hash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hash_size), &value_size, 0) &&
            hash_size == sizeof(digest) &&
            CryptGetHashParam(hash, HP_HASHVAL, digest, &digest_size, 0)) {
            hex = ToHexLower(digest, sizeof(digest));
        } else {
            failure = "CryptGetHashParam failed";
        }
    }

    if (hash != 0) {
        CryptDestroyHash(hash);
    }
    if (provider != 0) {
        CryptReleaseContext(provider, 0);
    }

    if (!failure.empty()) {
        SetError(error, failure);
        return {};
    }
    return hex;
}

} // namespace

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

std::string LauncherBackend::GenerateId(const std::string& prefix) {
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return prefix + "_" + std::to_string(now) + "_" + std::to_string(id_counter_++);
}

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
        auto trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '[') {
            continue;
        }
        const auto pos = trimmed.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        values[Trim(trimmed.substr(0, pos))] = Trim(trimmed.substr(pos + 1));
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
    if (it_current != values.end() && !Trim(it_current->second).empty()) {
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
            settings_.hotkey = GetStr(root, "hotkey", std::string("Alt+1"));
            settings_.execute_hide = GetBool(root, "executeHide", true);
            if (const auto* cg = FindField(root, "currentGroup"); cg != nullptr && cg->is_string()) {
                settings_.current_group = ToStdString(cg->as_string());
            }
            settings_.group_panel_width = GetF64(root, "groupPanelWidth", 220.0);
            settings_.main_window_width = GetF64(root, "mainWindowWidth", 1040.0);
            settings_.main_window_height = GetF64(root, "mainWindowHeight", 700.0);
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

bool LauncherBackend::SaveData(std::string* error) const {
    RotateBackupsBeforeSave();

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

    return WriteTextAtomic(data_path_, SerializeDocument(doc), error);
}

bool LauncherBackend::SaveSettings(std::string* error) const {
    ca::json::JsonDocument doc;
    auto& arena = doc.arena();
    ca::json::JsonValue root = ca::json::JsonValue::make_object();
    SetStr(root, arena, "hotkey", settings_.hotkey);
    SetBool(root, arena, "executeHide", settings_.execute_hide);
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

    PruneBackups(backup_dir);
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

std::vector<BackupEntry> LauncherBackend::ListBackups() const {    std::vector<BackupEntry> out;
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

    AppendJournal("delete_group", "id=" + group_id + " name=" + delete_it->name + " merged_into=" + target_group_id);
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
    if (!app_dir_.empty()) {
        const auto app_dir_text = app_dir_.string();
        const auto drive_root_text = app_dir_.root_path().string();
        ReplaceAllInPlace(&launch_target, "%pr%", app_dir_text);
        ReplaceAllInPlace(&launch_target, "%cr%", drive_root_text);
        ReplaceAllInPlace(&launch_args, "%pr%", app_dir_text);
        ReplaceAllInPlace(&launch_args, "%cr%", drive_root_text);
    }
    launch_target = ExpandEnvUtf8(launch_target);
    launch_args = ExpandEnvUtf8(launch_args);

    std::string launch_error;
    if (!launch_executor_->Launch(launch_target, launch_args, &launch_error)) {
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
