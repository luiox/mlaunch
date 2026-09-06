#pragma once

// 小工具插件层：纯逻辑，零 UI 依赖。
// 同一份 ToolDef 同时服务两个前端：
//   - mtool.exe CLI（给 AI / 脚本 / 管道）
//   - mlaunch 搜索框关键字（给人，实时预览 + 回车复制）
// 新增工具 = 新建一个 cpp 提供 BuildXxxTool()，并在 tool_registry.cpp 汇总。

#include <string>
#include <vector>

namespace tools {

// 结果列表中的一行：预览行不可选，selectable 行回车复制 copy_text。
struct ToolLine {
    std::string text;          // UTF-8 显示文本
    bool selectable = false;   // true 时可选中，回车复制 copy_text
    std::string copy_text;     // UTF-8，回车复制的内容
};

struct ToolOutput {
    bool ok = false;
    std::string error;             // UTF-8，ok=false 时的错误描述
    std::vector<ToolLine> lines;   // 预览行（无结果时可为空）
    std::string primary;           // 主结果（CLI 单值输出 / UI 默认复制内容）
};

// args 为 UTF-8 关键字之后的原始输入（已 trim 前后空白）。
using ToolRun = ToolOutput (*)(const std::string& args);

struct ToolDef {
    const char* keyword;   // 搜索关键字，全局唯一，如 "ts"
    const char* name;      // 中文名，如 "时间戳转换"
    const char* usage;     // 一行用法说明（含关键字本身）
    ToolRun run;
};

/// @brief 全部已注册工具（顺序即 UI / CLI --list 的展示顺序）。
const std::vector<ToolDef>& Registry();

/// @brief 按关键字查工具；未注册返回 nullptr。
const ToolDef* FindByKeyword(const std::string& keyword);

// —— 各工具的工厂声明（实现见同名 cpp）——
ToolDef BuildTimestampTool();   // ts
ToolDef BuildBase64Tool();      // b64
ToolDef BuildHashTool();        // hash <algo> <text>
ToolDef BuildMd5Tool();         // md5 <text>
ToolDef BuildSha1Tool();        // sha1 <text>
ToolDef BuildSha256Tool();      // sha256 <text>
ToolDef BuildUrlTool();         // url
ToolDef BuildUuidTool();        // uuid
ToolDef BuildSineTool();        // sine

} // namespace tools
