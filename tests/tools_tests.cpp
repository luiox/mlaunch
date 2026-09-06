// 小工具层测试：golden 值验证 + 往返一致性。不链接 DuiLib / shell32。

#include "tool_registry.h"

#include "libca/uuid/uuid.hpp"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

// 老工具 sin_data.txt 的 golden 数据（裸字符串字面量序列，见 sine_golden.inc 头注释）。
static const char kGoldenSinData[] =
#include "sine_golden.inc"
    ;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace {

using tools::FindByKeyword;
using tools::Registry;
using tools::ToolDef;
using tools::ToolOutput;

ToolOutput RunTool(const char* keyword, const std::string& args) {
    const ToolDef* tool = FindByKeyword(keyword);
    EXPECT_NE(tool, nullptr) << "工具未注册: " << keyword;
    if (tool == nullptr) {
        return {};
    }
    return tool->run(args);
}

// —— 注册表 ——

TEST(ToolRegistryTest, AllToolsRegistered) {
    ASSERT_FALSE(Registry().empty());
    EXPECT_NE(FindByKeyword("ts"), nullptr);
    EXPECT_NE(FindByKeyword("b64"), nullptr);
    EXPECT_NE(FindByKeyword("hash"), nullptr);
    EXPECT_NE(FindByKeyword("md5"), nullptr);
    EXPECT_NE(FindByKeyword("sha1"), nullptr);
    EXPECT_NE(FindByKeyword("sha256"), nullptr);
    EXPECT_NE(FindByKeyword("url"), nullptr);
    EXPECT_NE(FindByKeyword("uuid"), nullptr);
    EXPECT_EQ(FindByKeyword("nope"), nullptr);
}

TEST(ToolRegistryTest, KeywordsUnique) {
    for (std::size_t i = 0; i < Registry().size(); ++i) {
        for (std::size_t j = i + 1; j < Registry().size(); ++j) {
            EXPECT_NE(Registry()[i].keyword, Registry()[j].keyword)
                << "关键字重复: " << Registry()[i].keyword;
        }
    }
}

// —— 时间戳 ——

TEST(TimestampToolTest, KnownEpochToUtc) {
    // 黄金值只断 UTC 行（本地时区无关）：epoch 0 = 1970-01-01 00:00:00 UTC。
    const ToolOutput out = RunTool("ts", "0");
    ASSERT_TRUE(out.ok);
    ASSERT_GE(out.lines.size(), 2u);
    EXPECT_EQ(out.lines[1].text, "UTC  1970-01-01 00:00:00");
    EXPECT_EQ(out.lines[1].copy_text, "1970-01-01 00:00:00");
}

TEST(TimestampToolTest, MillisecondsDetected) {
    // 1788670404 = 2026-09-06 04:53:24 UTC；13 位输入按毫秒解释。
    const ToolOutput out = RunTool("ts", "1788670404000");
    ASSERT_TRUE(out.ok);
    ASSERT_GE(out.lines.size(), 2u);
    EXPECT_EQ(out.lines[1].text, "UTC  2026-09-06 04:53:24");
}

TEST(TimestampToolTest, DateToEpochRoundTrip) {
    // 本地时间 → epoch → 本地时间 往返一致（时区无关）。
    const ToolOutput to_epoch = RunTool("ts", "2026-09-06 12:53:24");
    ASSERT_TRUE(to_epoch.ok);
    const std::string epoch_str = to_epoch.primary;
    EXPECT_EQ(to_epoch.lines[0].copy_text, epoch_str);

    const ToolOutput back = RunTool("ts", epoch_str);
    ASSERT_TRUE(back.ok);
    // 回转后本地行文本应与原始输入一致。
    EXPECT_NE(back.lines[0].text.find("2026-09-06 12:53:24"), std::string::npos);
}

TEST(TimestampToolTest, EmptyShowsCurrent) {
    const ToolOutput out = RunTool("ts", "");
    ASSERT_TRUE(out.ok);
    ASSERT_FALSE(out.lines.empty());
    // 首行是时间戳（可复制），且为纯数字。
    const std::string& epoch = out.lines[0].copy_text;
    ASSERT_FALSE(epoch.empty());
    for (char ch : epoch) {
        ASSERT_GE(ch, '0');
        ASSERT_LE(ch, '9');
    }
}

TEST(TimestampToolTest, BadInputReportsError) {
    const ToolOutput out = RunTool("ts", "hello world");
    EXPECT_FALSE(out.ok);
    EXPECT_FALSE(out.error.empty());
}

// —— Base64 ——

TEST(Base64ToolTest, EncodeGolden) {
    const ToolOutput out = RunTool("b64", "e hello");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "aGVsbG8=");
}

TEST(Base64ToolTest, Utf8TextRoundTrip) {
    // 中文 UTF-8 字节级往返。
    const ToolOutput encoded = RunTool("b64", "e 中文内容 with spaces");
    ASSERT_TRUE(encoded.ok);
    const ToolOutput decoded = RunTool("b64", "d " + encoded.primary);
    ASSERT_TRUE(decoded.ok);
    EXPECT_EQ(decoded.primary, "中文内容 with spaces");
}

TEST(Base64ToolTest, DecodeInvalid) {
    const ToolOutput out = RunTool("b64", "d !!!not-base64!!!");
    EXPECT_FALSE(out.ok);
    EXPECT_FALSE(out.error.empty());
}

TEST(Base64ToolTest, MissingModeReportsUsage) {
    const ToolOutput out = RunTool("b64", "");
    EXPECT_FALSE(out.ok);
}

// —— 哈希 ——

TEST(HashToolTest, Md5Golden) {
    const ToolOutput out = RunTool("md5", "hello");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "5d41402abc4b2a76b9719d911017c592");
}

TEST(HashToolTest, Sha1Golden) {
    const ToolOutput out = RunTool("sha1", "hello");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
}

TEST(HashToolTest, Sha256Golden) {
    const ToolOutput out = RunTool("sha256", "hello");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

TEST(HashToolTest, HashKeywordWithAlgo) {
    const ToolOutput out = RunTool("hash", "sha256 hello");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

TEST(HashToolTest, HashKeywordBadAlgo) {
    const ToolOutput out = RunTool("hash", "crc32 hello");
    EXPECT_FALSE(out.ok);
}

// —— URL ——

TEST(UrlToolTest, EncodeGolden) {
    const ToolOutput out = RunTool("url", "e a b/c?d=中");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "a%20b%2Fc%3Fd%3D%E4%B8%AD");
}

TEST(UrlToolTest, DecodeGolden) {
    const ToolOutput out = RunTool("url", "d https://www.baidu.com/s?wd=%E4%B8%AD%E6%96%87");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "https://www.baidu.com/s?wd=中文");
}

TEST(UrlToolTest, RoundTrip) {
    const ToolOutput encoded = RunTool("url", "e path+with+plus & symbols~");
    ASSERT_TRUE(encoded.ok);
    const ToolOutput decoded = RunTool("url", "d " + encoded.primary);
    ASSERT_TRUE(decoded.ok);
    EXPECT_EQ(decoded.primary, "path+with+plus & symbols~");
}

TEST(UrlToolTest, InvalidEscapeKeptAsIs) {
    const ToolOutput out = RunTool("url", "d 100%");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(out.primary, "100%");
}

// —— UUID ——

TEST(UuidToolTest, GeneratesValidV4) {
    const ToolOutput out = RunTool("uuid", "");
    ASSERT_TRUE(out.ok);
    EXPECT_TRUE(ca::uuid::is_valid(out.primary));
}

TEST(UuidToolTest, BatchCount) {
    const ToolOutput out = RunTool("uuid", "5");
    ASSERT_TRUE(out.ok);
    // 5 行 uuid + 1 行汇总。
    ASSERT_EQ(out.lines.size(), 6u);
    EXPECT_EQ(out.lines.back().selectable, false);
    std::set<std::string> ids;
    for (std::size_t i = 0; i < 5; ++i) {
        ids.insert(out.lines[i].copy_text);
    }
    EXPECT_EQ(ids.size(), 5u);
}

// —— 正弦波（golden：老工具 sin_data.txt，A=2048 f0=10 fs=500 phi=0 t=1，500 点）——

namespace {
// 逐值比对，返回 {一致点数, 最大偏差}。
std::pair<int, int> CompareGolden(const std::string& joined, const char* golden) {
    std::vector<int> got;
    std::size_t begin = 0;
    while (begin <= joined.size()) {
        const std::size_t comma = joined.find(',', begin);
        const std::string tok = joined.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin);
        if (tok.empty()) break;
        got.push_back(std::atoi(tok.c_str()));
        if (comma == std::string::npos) break;
        begin = comma + 1;
    }

    std::vector<int> want;
    begin = 0;
    const std::string g(golden);
    while (begin <= g.size()) {
        const std::size_t comma = g.find(',', begin);
        const std::string tok = g.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin);
        if (tok.empty()) break;
        want.push_back(std::atoi(tok.c_str()));
        if (comma == std::string::npos) break;
        begin = comma + 1;
    }

    if (got.size() != want.size()) {
        return {0, 1 << 30};
    }
    int match = 0;
    int max_delta = 0;
    for (std::size_t i = 0; i < want.size(); ++i) {
        const int delta = std::abs(got[i] - want[i]);
        max_delta = std::max(max_delta, delta);
        if (delta == 0) ++match;
    }
    return {match, max_delta};
}
} // namespace

TEST(SineToolTest, GoldenAlignment) {
    // 老工具的浮点指纹：3 处过零点相差 1 LSB（2047 vs 2048），其余位级一致。
    // 锁定"点数一致 + 至少 495 点位级一致 + 最大偏差 ≤ 1"，
    // 任何公式退化（幅度错、相位错、钳位错）都会被最大偏差/一致率抓住。
    const ToolOutput out = RunTool("sine", "--amp 2048 --f0 10 --fs 500 --phi 0 --t 1 --data");
    ASSERT_TRUE(out.ok);
    const auto [match, max_delta] = CompareGolden(out.primary, kGoldenSinData);
    EXPECT_EQ(match, 497);
    EXPECT_EQ(max_delta, 1);
}

TEST(SineToolTest, SampleCountFromFsTimesT) {
    const ToolOutput out = RunTool("sine", "--fs 100 --f0 1 --t 2 --points 0 --data");
    ASSERT_TRUE(out.ok);
    EXPECT_EQ(std::count(out.primary.begin(), out.primary.end(), ',') + 1, 200u);
}

TEST(SineToolTest, PointsOverride) {
    const ToolOutput out = RunTool("sine", "--points 8 --data");
    ASSERT_TRUE(out.ok);
    // 缺省 f0=10 fs=500 → 每周期 50 点，8 点落在首周期上升段，单调不减。
    EXPECT_EQ(out.primary, "2048,2304,2557,2801,3034,3251,3449,3626");
}

TEST(SineToolTest, DefaultOutputHasArtAndCopyLine) {
    const ToolOutput out = RunTool("sine", "");
    ASSERT_TRUE(out.ok);
    ASSERT_GE(out.lines.size(), 4u);
    // 首行参数摘要；末行可复制完整数据。
    EXPECT_NE(out.lines[0].text.find("A=2048"), std::string::npos);
    EXPECT_EQ(out.lines.back().selectable, true);
    EXPECT_EQ(out.lines.back().copy_text.size(), out.primary.size());
}

} // namespace
