// mtool.exe —— 小工具命令行入口（给 AI / 脚本 / 管道用）。
// 约定：stdout 输出结果（UTF-8），stderr 输出错误，exit 0 成功 / 1 执行失败 / 2 用法错误。
// 参数统一经 CommandLineToArgvW 拿 UTF-16 再转 UTF-8，避免 ANSI 代码页中文乱码。

#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "tool_registry.h"

namespace {

// UTF-16 → UTF-8。不引 mlaunch-core 的 string_util，保持 CLI 只依赖 tools 层。
std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), result.data(),
                          size, nullptr, nullptr);
    return result;
}

std::vector<std::string> ArgsToUtf8() {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    if (argv != nullptr) {
        // i 从 1 开始跳过 argv[0]（程序自身路径）。
        for (int i = 1; i < argc; ++i) {
            args.push_back(WideToUtf8(argv[i]));
        }
        LocalFree(argv);
    }
    return args;
}

void PrintUsage() {
    std::printf("mtool — mlaunch 小工具命令行\n\n用法: mtool <关键字> [参数...]\n\n可用工具:\n");
    for (const auto& tool : tools::Registry()) {
        std::printf("  %-8s %s\n           %s\n", tool.keyword, tool.name, tool.usage);
    }
}

} // namespace

int main() {
    ::SetConsoleOutputCP(CP_UTF8);

    const std::vector<std::string> args = ArgsToUtf8();
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        PrintUsage();
        return 0;
    }

    const tools::ToolDef* tool = tools::FindByKeyword(args[0]);
    if (tool == nullptr) {
        std::fprintf(stderr, "mtool: 未知工具 '%s'（mtool --help 查看列表）\n", args[0].c_str());
        return 2;
    }

    // 关键字之后的参数以单空格拼接（工具内部自行二次拆词）。
    std::string text;
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (i > 1) {
            text += ' ';
        }
        text += args[i];
    }

    const tools::ToolOutput out = tool->run(text);
    if (!out.ok) {
        std::fprintf(stderr, "mtool %s: %s\n", tool->keyword, out.error.c_str());
        return 1;
    }

    // 结果行逐行打印；可复制行（selectable）在 CLI 里就是普通输出行。
    for (const auto& line : out.lines) {
        std::fwrite(line.text.data(), 1, line.text.size(), stdout);
        std::fputc('\n', stdout);
    }
    return 0;
}
