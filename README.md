# mlaunch

`C++ + DuiLib + xmake` 实现的键盘启动器（Poner 的 C++ 重写版）。

## 目录结构

- `src/core/` — 纯 CRUD 核心：数据模型、JSON 持久化、备份轮转、journal、软删除/撤销。零 UI 依赖，可独立测试。
- `src/ui/` — DuiLib 界面层：窗口、列表控制器、搜索、对话框、图标、shell 服务实现。
- `tests/core_tests.cpp` — 核心测试（不链接 DuiLib/shell32，注入 fake 执行器）。
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
```

## 依赖

- [DuiLib_DuiEditor](https://github.com/luiox/DuiLib_DuiEditor)（submodule）— UI 框架
- [libca](https://github.com/luiox/libca)（submodule）— JSON 读写（`libca_json`）等基础库
- `libicon-core/` — 顶栏 SVG 图标资产
- gtest — 测试框架

## 架构说明

`core` 与 `ui` 通过两个接口解耦：

- `core::LaunchExecutor` — 进程启动（UI 侧实现为 `ShellExecuteExW`，测试注入 fake）
- `core::ShortcutResolver` — `.lnk` 解析（UI 侧实现为 `IShellLinkW`，测试注入 fake）

数据文件：`launcher.v2.json`（数据）、`nassistant.settings.json`（设置）、
`backups/`（滚动备份 5 份 + 每日快照 30 份）、`operations.log`（操作 journal）。
