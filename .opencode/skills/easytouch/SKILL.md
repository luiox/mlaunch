---
name: easytouch
description: Use EasyTouch (`et` CLI) for Windows GUI verification and automation in this repo — screen capture of the mlaunch window, UI screenshot comparison against docs/poner_ui_reference.png, window find/activate, wait-for-window/pixel, clipboard and mouse/keyboard control. Use whenever asked to 截图, screenshot, 看看 UI, verify visually, or when a build needs visual regression checking.
---

# EasyTouch — GUI 截图与自动化

`et` 是跨平台系统自动化 CLI（已全局安装，直接调用 `et`）。本项目用它做 UI 截图验证，替代手写 PowerShell 截图脚本。

## 核心工作流（本项目）

构建后截图验证 UI（对照 `docs/poner_ui_reference.png` 的 Poner 原版基线）：

```bash
# 1. 启动应用（如未运行）
Start-Process "build/windows/x64/release/mlaunch.exe"

# 2. 找到主窗口句柄
et window find --title Poner --match exact

# 3. 截图（全屏截图后裁剪，或直接截屏）
et screen capture --path build/shot_latest.png

# 4. 等待窗口出现（自动化脚本中先等待再操作）
et wait window --title Poner --timeout-ms 5000

# 5. 读取像素颜色验证主题色（如分组区应为白色 FFFFFF）
et screen pixel-color --x 100 --y 300
```

截图文件用 Read 工具查看（支持图片），与 `docs/poner_ui_reference.png` 对比。

## 命令速查

### 系统信息
- `et system os-info` / `cpu-info` / `memory-info` / `disk-list` / `process-list` / `hardware-info` / `network-info`

### 窗口
- `et window list [--include-hidden] [--pid <pid>]`
- `et window find --title <text> [--match <contains|exact>]`
- `et window activate --handle <handle>` / `window close --handle <handle>` / `window foreground`

### 屏幕
- `et screen displays`
- `et screen capture [--path <file>]`
- `et screen pixel-color --x <x> --y <y>`

### 键鼠
- `et mouse position` / `move --x --y` / `click [--button left|right|middle]` / `scroll --delta <n>`
- `et keyboard key --key <name>` / `hotkey --keys <combo>` / `type --text <value>` / `paste`

### 剪贴板
- `et clipboard get-text` / `set-text --text <v>` / `set-image --path <file>` / `set-files --paths <a;b>`

### 等待（自动化脚本的关键步骤）
- `et wait window --title <t> [--timeout-ms <ms>] [--match contains|exact]`
- `et wait pixel --x --y --hex RRGGBB`（验证某处颜色变化）
- `et wait process --name <n> [--expect-running true|false]`
- `et wait clipboard [--expect-text <v>]`

## 注意事项

- 推荐节奏：**观察 → 操作 → 等待确认**，不要盲操作
- 脚本化统一加 `--output json` 便于解析
- `wait` 默认超时 2000ms，按需加 `--timeout-ms`
- 窗口匹配模式统一 `contains|exact`
- 高风险操作（点击/键盘/窗口关闭）前先观察等待
- 失败时看 `failure.code` / `failure.message` / `failure.detail`

## MCP 方式（可选）

`et mcp-stdio` 可作为 MCP server 接入 opencode.json：

```json
{
  "mcp": {
    "easytouch": { "type": "local", "command": ["et", "mcp-stdio"], "enabled": true }
  }
}
```
