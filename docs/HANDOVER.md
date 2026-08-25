# mlaunch 开发交接文档

> 更新：2026-08-25。本文档供新会话快速接续开发，读完即可上手。
> 配套阅读：`后续计划列表.md`（总计划与勾选状态）。

## 一、项目现状

- VB6 Poner 启动器的 C++/DuiLib 重写，独立仓库 `D:\WorkSpace\mlaunch`（GitHub luiox/mlaunch）。
- 架构：`src/core`（零 DuiLib/shell32 依赖，可单测）+ `src/ui`（DuiLib）。
- submodule：`third_party/DuiLib_DuiEditor`（**注意：是 DuiEditor fork，行为与原版 DuiLib 有差异，见第三节**）、`third_party/libca`（JSON 用 ca::json）。
- 数据目录 `%LOCALAPPDATA%\nassistant\`：`launcher.v2.json`（已导入真实 Poner 数据：12 组 171 条）、`backups/`（滚动5+每日30）、`operations.log`（journal）、`ui_state.ini`、`nassistant.log`。
- 已完成：M0 数据安全层（备份轮转/软删除+回收站/撤销/journal/损坏恢复）、UI 对齐 Poner（字体/配色/Toast/单击启动）、P0-M 幂等迁移器（ImportPonerData）、**项目编辑窗 MVP（本次完成，commit 99b97ca）**。
- 测试：`tests/core_tests.cpp` 19/19 通过。

## 二、构建 / 运行 / 测试

```powershell
cd D:\WorkSpace\mlaunch
xmake f -p windows -a x64 -m release   # 首次
xmake build mlaunch                     # 主程序
xmake build core_tests; xmake run core_tests   # 19 个用例
# exe: build\windows\x64\release\mlaunch.exe
```

## 三、DuiLib fork 的关键坑（血泪教训，务必先读）

1. **`WindowImplBase::MessageHandler` 是死代码**。本 fork 的消息链是
   `CWindowWin32::__WndProc → HandleMessage → (switch→OnXxx) → HandleCustomMessage(虚) → m_pm.MessageHandler`。
   重写 `MessageHandler` 永远不会被调用。**键盘/自定消息一律重写 `HandleCustomMessage`**（参考 `app_window_interaction.cpp` 与 `item_edit_window.cpp`）。
2. **CEditUI 的原生 EDIT 子窗口（类名 `EditWnd`，不是 `Edit`！）**：
   - 失焦即自毁（`CEditWndWin32::OnKillFocus → PostMessage(WM_CLOSE)`）；
   - 重新激活时 `CPaintManagerUI::SetFocus` 因 `pControl==m_pFocus` 早退，SETFOCUS 事件不发、原生 EDIT 不会重建；
   - `CEditUI::DoEvent` **没有 UIEVENT_CHAR 分支**——原生 EDIT 不存在时输入直接丢弃；
   - 解决：`item_edit_window.cpp` 的 `EnsureNativeEditFocused()`（清焦点→设回→必要时模拟 BUTTONDOWN）+ `kFocusEditMsg` 延迟聚焦（WM_ACTIVATE 后系统还会发 WM_SETFOCUS 抢回焦点，必须 PostMessage 延迟处理）。
   - 真实鼠标点击路径（BUTTONDOWN 且 IsFocused）是可靠的；主窗搜索框正常。
3. **静态链接必须定义 `UILIB_STATIC`**（mlaunch target 已加）。否则 `UILIB_API=dllimport`，头文件 inline 类（如 `CDuiPoint(POINT)`）会 LNK2019。
4. MSVC 必须 `/utf-8`（已在 xmake.lua），否则 `_T("微软雅黑")` 按 GBK 误解导致字体回退。
5. fork 源码含 GBK 注释，rg/grep 输出会乱码（如 "SetFocus" 显示为 "n"），用 `[System.IO.File]::ReadAllLines` + `-match` 代替。

## 四、环境与自动化（et / EasyTouch）

- `et` CLI 已全局安装（skill：`.opencode/skills/easytouch/SKILL.md`）。截图工具：`tools/shot.ps1 -TitlePattern <t> -Out <png>`（PrintWindow，不受遮挡影响）。
- **OpenCode 终端是置顶窗口**，遮挡屏幕约 x∈[230,1440]（1440x900）：鼠标点击会落在终端上，**键盘 SendInput 可正常到达前台窗口**。UI 自动化用纯键盘（Shift+F10 开上下文菜单）。
- **`et window find --title "Poner"` 会匹配到真 Poner**（`D:\desktop\poner\Poner.exe`，用户可能开着它对照 UI，标题同名）！**必须 `--pid <mlaunch pid>` 限定**，否则会驱动到原版 Poner（本次曾把它点崩溃，抱歉）。
- 稳定的"打开编辑对话框"自动化序列（已验证）：
  1. `et window find --title Poner --pid <pid>` 取主窗 handle/bounds
  2. `et window activate --handle <主窗>`
  3. `et mouse move --x (left+178) --y (top+52)`（光标放第一行，Shift+F10 的菜单按光标位置分流：item 行→item 菜单，否则主菜单——主菜单 Down×6+Enter 会选中 Exit 退出程序！）
  4. `et keyboard hotkey shift+f10` → `down ×6` → `enter`（item 菜单第 6 项 = Edit Item）
  5. 对话框激活后**每次 bash 调用之间前台可能被终端抢走**：发键前先 `et window activate --handle <对话框>` 并在同一条命令里立即发键。
- item 菜单结构：0 Run as administrator / 1 Open file location / 2 Explorer menu / 3 Copy full path / 4 分隔 / 5 Add Item / 6 **Edit Item** / 7 Delete Item / 8 Move To Group。
- 已验证的编辑流（2026-08-25）：打开→输入→Enter→`operations.log` 出现 `update_item`→对话框关闭。Ctrl+A 在自动化下不生效（追加而非替换），真实点击路径无此问题。

## 五、剩余工作（按 后续计划列表.md 顺序）

1. **主菜单占位转正**：Sort By Name / Export / Settings 目前是占位（`ShowMainContextMenu`，app_window.cpp:720）。
2. **设置窗 MVP**：热键、启动后隐藏、默认宽高、分组栏宽度。
3. **文案中文化**：菜单/Toast/对话框仍有英文（"Add Item"、"select an item first" 等）。
4. **P2**：
   - Explorer 右键菜单（kItemShellMenu 已有菜单项，未实现）
   - 无边框窗口阴影
   - 像素级对齐：列表行高、图标灰底框去除（对照 `docs/poner_ui_reference.png`）
5. 编辑窗可选增强：Tab 焦点轮换、Ctrl+A 全选（原生 EDIT 已支持，需焦点在 EDIT 内）。

## 六、本次会话（2026-08-25）改动摘要（commit 99b97ca）

- 新增 `src/ui/item_edit_window.{h,cpp}`：独立弹出编辑窗（WindowImplBase 子类）。
- `dialog_manager.{h,cpp}`：OpenItemDialog 创建弹出窗；OnItemEditDone 回调按 item_id 走 UpsertItem 更新。
- `app_window.*`：菜单/快捷键入口接 OpenItemDialog(true/false)；legacy_root 捕获；%pr%/%cr% 展开。
- `shell_services.*`：PickOpenPath(HWND, filter) 文件选择对话框。
- `core`：ItemInput/UpsertItem 支持 id 更新；journal `update_item`。
- `xmake.lua`：mlaunch target 加 `UILIB_STATIC`。
- 测试数据说明：自动化测试曾污染 `%LOCALAPPDATA%\nassistant\launcher.v2.json`，已用 `backups/launcher.v2.20260824.json` 恢复（171 条干净状态）。

## 七、快速自检清单（新会话开始时）

```powershell
cd D:\WorkSpace\mlaunch
git log --oneline -3          # 应看到 99b97ca 编辑窗提交
xmake build mlaunch           # 应 build ok
xmake run core_tests          # 应 19 tests PASSED
```
UI 验证：运行 mlaunch → 右键（或 Shift+F10）任一启动项 → Edit Item → 改名 → 确定 → 检查 `%LOCALAPPDATA%\nassistant\operations.log` 出现 `update_item`。
