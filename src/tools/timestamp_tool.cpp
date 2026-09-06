// 时间戳转换工具：替代 D:\资料\工具软件\6_时间戳转换工具。
// 纯标准库实现：epoch ↔ 本地/UTC 时间字符串；输入输出全部 UTF-8。

#include "tool_registry.h"

#include <cctype>
#include <cstdio>
#include <ctime>
#include <string>

#include "tool_util.h"

namespace tools {
namespace {

using internal::CopyLine;
using internal::TextLine;
using internal::Trim;

bool AllDigits(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    for (char ch : s) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

// epoch → "yyyy-MM-dd HH:mm:ss"。utc=false 时走本地时区。
std::string FormatEpoch(long long epoch, bool utc) {
    const std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm {};
    if (utc) {
        gmtime_s(&tm, &t);
    } else {
        localtime_s(&tm, &t);
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

std::string WeekdayCn(int tm_wday) {
    static const char* kDays[] = {"日", "一", "二", "三", "四", "五", "六"};
    return std::string("周") + kDays[tm_wday % 7];
}

// "yyyy-MM-dd[ HH:mm[:ss]]"（本地时区）→ epoch 秒。
// 缺省时间视为当天 00:00:00；mktime 依赖系统时区，行为与用户直觉一致。
bool ParseLocalToEpoch(const std::string& text, long long* out_epoch) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    const int consumed = std::sscanf(text.c_str(), "%d-%d-%d%*[T ]%d:%d:%d", &y, &mo, &d, &h, &mi, &s);
    // 允许：完整 6 字段 / 5 字段（无秒）/ 3 字段（仅日期）。
    if (consumed != 6 && consumed != 5 && consumed != 3) {
        return false;
    }
    if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31 ||
        h < 0 || h > 23 || mi < 0 || mi > 59 || s < 0 || s > 59) {
        return false;
    }
    std::tm tm {};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = s;
    tm.tm_isdst = -1;
    const std::time_t t = std::mktime(&tm);
    if (t == static_cast<std::time_t>(-1)) {
        return false;
    }
    *out_epoch = static_cast<long long>(t);
    return true;
}

// 本地时间行：展示带星期，复制值保持纯净（可直接粘贴使用）。
ToolLine LocalLine(long long epoch, const std::tm& local) {
    ToolLine line;
    line.text = std::string("本地  ") + FormatEpoch(epoch, false) + " " + WeekdayCn(local.tm_wday);
    line.selectable = true;
    line.copy_text = FormatEpoch(epoch, false);
    return line;
}

ToolOutput Run(const std::string& raw_args) {
    ToolOutput out;
    const std::string args = Trim(raw_args);

    // 输入分类：纯数字 = 时间戳（>=13 位按毫秒，如 Java/JS 的 Date.now()）；
    // 含 '-' = 日期时间（本地时区）；空 / now = 当前时刻。
    if (args.empty() || args == "now") {
        const std::time_t now = std::time(nullptr);
        const long long epoch = static_cast<long long>(now);
        std::tm local {};
        localtime_s(&local, &now);
        out.lines.push_back(CopyLine("时间戳", std::to_string(epoch)));
        out.lines.push_back(LocalLine(epoch, local));
        out.lines.push_back(CopyLine("UTC", FormatEpoch(epoch, true)));
        out.primary = std::to_string(epoch);
    } else if (AllDigits(args)) {
        const long long value = std::stoll(args);
        // 13 位数字已超过 2286 年的秒级范围，按毫秒解释（Java/JS 惯例）。
        const long long epoch = args.size() >= 13 ? value / 1000 : value;
        const std::time_t t = static_cast<std::time_t>(epoch);
        std::tm local {};
        localtime_s(&local, &t);
        out.lines.push_back(LocalLine(epoch, local));
        out.lines.push_back(CopyLine("UTC", FormatEpoch(epoch, true)));
        out.primary = FormatEpoch(epoch, false);
    } else if (args.find('-') != std::string::npos) {
        long long epoch = 0;
        if (!ParseLocalToEpoch(args, &epoch)) {
            out.error = "无法解析日期：" + args + "（格式 yyyy-MM-dd[ HH:mm[:ss]]）";
            return out;
        }
        out.lines.push_back(CopyLine("时间戳", std::to_string(epoch)));
        out.lines.push_back(CopyLine("UTC", FormatEpoch(epoch, true)));
        out.primary = std::to_string(epoch);
    } else {
        out.error = "无法识别输入：" + args;
        return out;
    }

    out.ok = true;
    return out;
}

} // namespace

ToolDef BuildTimestampTool() {
    ToolDef def;
    def.keyword = "ts";
    def.name = "时间戳转换";
    def.usage = "ts [时间戳|yyyy-MM-dd HH:mm:ss]，无参数显示当前时间";
    def.run = &Run;
    return def;
}

} // namespace tools
