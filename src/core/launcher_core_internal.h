#pragma once

// launcher_core 各编译单元共享的内部辅助函数。
// 仅由 core 自己的 .cpp 包含，不属于公共 API；全部 inline，避免跨单元链接管理。

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <wincrypt.h>

#include "libca/json/json.hpp"

#include "logger.h"
#include "utils/string_util.h"

namespace core {

// ---- libca_json 访问辅助：字段缺失/类型不符时回退默认值 ----

inline std::string ToStdString(const ca::str::Utf8StringRef& ref) {
    return std::string(reinterpret_cast<const char*>(ref.data()), ref.byte_length());
}

inline void ReplaceAllInPlace(std::string* text, const std::string& from, const std::string& to) {
    if (from.empty() || text == nullptr) {
        return;
    }
    std::size_t pos = 0;
    while ((pos = text->find(from, pos)) != std::string::npos) {
        text->replace(pos, from.size(), to);
        pos += to.size();
    }
}

inline std::string ExpandEnvUtf8(const std::string& text) {
    std::wstring wide = launcher::util::Utf8ToWide(text);
    wchar_t buffer[32768]{};
    const DWORD size = ::ExpandEnvironmentStringsW(wide.c_str(), buffer, 32768);
    if (size == 0 || size > 32768) {
        return text;
    }
    return launcher::util::WideToUtf8(buffer);
}

inline const ca::json::JsonValue* FindField(const ca::json::JsonValue& object, const char* key) {
    return object.find(ca::str::Utf8StringRef::from_cstr(key));
}

inline std::string GetStr(const ca::json::JsonValue& object, const char* key, std::string fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_string()) {
        return ToStdString(v->as_string());
    }
    return fallback;
}

inline std::uint64_t GetU64(const ca::json::JsonValue& object, const char* key, std::uint64_t fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_number()) {
        return static_cast<std::uint64_t>(v->as_int_or(static_cast<ca::i64>(fallback)));
    }
    return fallback;
}

inline int GetInt(const ca::json::JsonValue& object, const char* key, int fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_number()) {
        return static_cast<int>(v->as_int_or(fallback));
    }
    return fallback;
}

inline bool GetBool(const ca::json::JsonValue& object, const char* key, bool fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_bool()) {
        return v->as_bool();
    }
    return fallback;
}

inline double GetF64(const ca::json::JsonValue& object, const char* key, double fallback) {
    if (const auto* v = FindField(object, key); v != nullptr && v->is_number()) {
        return v->as_float_or(fallback);
    }
    return fallback;
}

// 解析 JSON 文本；失败时返回 nullopt 并给出错误描述。
inline std::optional<ca::json::JsonDocument> ParseJsonText(const std::string& text, std::string* parse_error) {
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
inline void SetStr(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, const std::string& value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_string(arena.intern(value.c_str())));
}

inline void SetInt(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, std::int64_t value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_int(value));
}

inline void SetBool(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, bool value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_bool(value));
}

inline void SetF64(ca::json::JsonValue& object, ca::str::Utf8StringArena& arena, const char* key, double value) {
    object.set(arena.intern(key), ca::json::JsonValue::make_float(value));
}

inline std::string SerializeDocument(const ca::json::JsonDocument& document) {
    ca::json::JsonWriterOptions options;
    options.pretty = true;
    options.indent = 2;
    const auto text = ca::json::JsonWriter::write(document, options);
    return std::string(reinterpret_cast<const char*>(text.data()), text.byte_length());
}

inline void SetError(std::string* out, const std::string& msg) {
    if (out) {
        *out = msg;
    }
}

inline std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline bool WriteTextAtomic(const std::filesystem::path& path, const std::string& content, std::string* error) {
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

inline bool BackupCorruptedJson(const std::filesystem::path& path) {
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

inline std::tm ToLocalTime(std::chrono::system_clock::time_point tp) {
    const auto time_t_value = std::chrono::system_clock::to_time_t(tp);
    std::tm local{};
    localtime_s(&local, &time_t_value);
    return local;
}

inline std::string FormatJournalTimestamp(std::chrono::system_clock::time_point tp) {
    const auto local = ToLocalTime(tp);
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        local.tm_hour, local.tm_min, local.tm_sec);
    return buffer;
}

inline std::string FormatFileStamp(std::chrono::system_clock::time_point tp) {
    const auto local = ToLocalTime(tp);
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d-%02d%02d%02d",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        local.tm_hour, local.tm_min, local.tm_sec);
    return buffer;
}

inline std::string FormatDateStamp(std::chrono::system_clock::time_point tp) {
    const auto local = ToLocalTime(tp);
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    return buffer;
}

constexpr std::size_t kRollingStampLength = 15; // YYYYMMDD-HHMMSS
constexpr std::size_t kDailyStampLength = 8;    // YYYYMMDD

// Returns "rolling", "daily" or "" for non-backup files.
inline std::string ClassifyBackupName(const std::string& file_name) {
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

inline void PruneBackups(const std::filesystem::path& backup_dir) {
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

inline std::string ToHexLower(const unsigned char* data, std::size_t size) {
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i) {
        out.push_back(kDigits[data[i] >> 4]);
        out.push_back(kDigits[data[i] & 0x0F]);
    }
    return out;
}

inline std::string ComputeMd5Hex(const std::string& content, std::string* error) {
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

} // namespace core
