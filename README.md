# mlaunch

`C++ + DuiLib + xmake` 实现的键盘启动器（Poner 的 C++ 重写版）。

## 目录结构

- `src/core/` — 纯 CRUD 核心：数据模型、JSON 持久化、备份轮转、journal、软删除/撤销。零 UI 依赖，可独立测试。
- `src/tools/` — 小工具插件层：纯函数注册表（时间戳、base64、hash、url、uuid），CLI 与搜索框共用。零 UI 依赖，可独立测试。
- `tools/` — `mtool` 命令行入口（`mtool <关键字> [参数]`，给 AI / 脚本 / 管道用）。
- `src/ui/` — DuiLib 界面层：窗口、列表控制器、搜索、对话框、图标、shell 服务实现。
- `tests/core_tests.cpp` — 核心测试（不链接 DuiLib/shell32，注入 fake 执行器）。
- `tests/tools_tests.cpp` — 小工具层测试（golden 值 + 往返一致性）。
- `docs/` — 开发计划、功能清单、VB6 UI 参考截图。

## 构建

```powershell
git submodule update --init --recursive
xmake f -p windows -a x64 -m release
xmake
xmake run mlaunch
```

## 测试

```powershell
xmake build core_tests
xmake run core_tests
xmake build tools_tests
xmake run tools_tests
```

## 小工具（plugin 机制 v1）

同一份工具注册表（`src/tools/tool_registry.h`）服务两个入口：

- **搜索框**（给人）：搜索模式输入 `ts 1788670404`、`b64 e 文本`、`md5 文本`、
  `url d %E4%B8%AD`、`uuid`、`sine --f0 2 --fs 64 --points 32`（ASCII 波形图预览）、
  `font 中`（点阵字形预览）等，实时预览结果，回车复制选中行。
  `sine` 兼容老工具 sin_config.txt（`sine --config 路径 --data`）；`font` 与
  PCtoLCD2002 取模结果逐字节一致（'中'字 16x16 golden 测试锁定）。
- **mtool.exe**（给 AI / 脚本）：`mtool ts 1788670404`；stdout 出结果（UTF-8）、
  stderr 出错误、exit 0/1/2 区分成功/失败/用法错误；`mtool --help` 列出全部工具。

新增工具：在 `src/tools/` 新建 cpp 实现 `Run` 函数，提供 `BuildXxxTool()` 工厂，
并在 `tool_registry.cpp` 的 `Registry()` 中注册即可，CLI 与搜索框同时生效。

## 依赖

- [DuiLib_DuiEditor](https://github.com/luiox/DuiLib_DuiEditor)（submodule）— UI 框架
- [libca](https://github.com/luiox/libca)（submodule）— JSON 读写（`libca_json`）等基础库
- [micon](https://github.com/luiox/micon)（submodule）— 统一风格 SVG 图标库（顶栏图标资产）
- gtest — 测试框架

## 架构说明

`core` 与 `ui` 通过两个接口解耦：

- `core::LaunchExecutor` — 进程启动（UI 侧实现为 `ShellExecuteExW`，测试注入 fake）
- `core::ShortcutResolver` — `.lnk` 解析（UI 侧实现为 `IShellLinkW`，测试注入 fake）

数据文件：`launcher.v2.json`（数据）、`nassistant.settings.json`（设置）、
`backups/`（滚动备份 5 份 + 每日快照 30 份）、`operations.log`（操作 journal）。
