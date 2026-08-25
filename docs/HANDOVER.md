# mlaunch 开发交接文档

> 更新：2026-08-25（第二次，设置窗/主菜单/中文化会话）。本文档供新会话快速接续开发，读完即可上手。
> 配套阅读：`后续计划列表.md`（总计划与勾选状态）。

## 一、项目现状

- VB6 Poner 启动器的 C++/DuiLib 重写，独立仓库 `D:\WorkSpace\mlaunch`（GitHub luiox/mlaunch）。
- 架构：`src/core`（零 DuiLib/shell32 依赖，可单测）+ `src/ui`（DuiLib）。
- submodule：`third_party/DuiLib_DuiEditor`（**注意：是 DuiEditor fork，行为与原版 DuiLib 有差异，见第三节**）、`third_party/libca`（JSON 用 ca::json）。
- 数据目录 `%LOCALAPPDATA%\nassistant\`：`launcher.v2.json`（已导入真实 Poner 数据：12 组 171 条）、`backups/`（滚动5+每日30）、`operations.log`（journal）、`nassistant.settings.json`、`ui_state.ini`、`nassistant.log`。
- 已完成：M0 数据安全层、UI 对齐 Poner、P0-M 幂等迁移器、项目编辑窗 MVP（99b97ca）、**设置窗 MVP + 主菜单转正 + 全量中文化（本次，见第六节）**。
- 测试：`tests/core_tests.cpp` 22/22 通过。

## 二、构建 / 运行 / 测试

```powershell
cd D:\WorkSpace\mlaunch
xmake f -p windows -a x64 -m release   # 首次
xmake build mlaunch                     # 主程序（注意：构建前先关掉在跑的 mlaunch，否则 LNK1104 文件锁）
xmake build core_tests; xmake run core_tests   # 22 个用例
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

- `et` CLI 已全局安装（skill：`.opencode/skills/easytouch/SKILL.md`）。截图工具：`tools/shot.ps1 -TitlePattern <t> -Out <png>`（PrintWindow，不受遮挡影响；用 `pwsh -File` 调，本机无 `powershell`）。
- **OpenCode 终端是置顶窗口**，遮挡屏幕约 x∈[230,1440]（1440x900）：鼠标点击会落在终端上，**键盘 SendInput 可正常到达前台窗口**。UI 自动化用纯键盘。
- **`et window find --title "Poner"` 会匹配到真 Poner**（`D:\desktop\poner\Poner.exe`，用户可能开着它对照 UI，标题同名）！**必须 `--pid <mlaunch pid>` 限定**。
- **et 键盘语法坑**：必须 `et keyboard hotkey --keys ctrl+m`（`--keys` 不能省！`et keyboard hotkey ctrl+m` 会 invalid_args 且被 Out-Null 吞掉，表现为"键发了没反应"）。单键 `et keyboard key --key down|enter|esc`。
- **本 fork 吞掉 Shift+F10**（前台正确也不产生 WM_CONTEXTMENU）。已给应用加了键盘菜单入口：**Ctrl+M 或 Apps 键** → 按光标位置路由分组/条目/主菜单。
- 稳定的"打开主菜单"自动化序列（已验证）：
  1. `et window find --title Poner --pid <pid>` 取 handle
  2. `et window activate --handle <h>` → `et window foreground` **校验 handle 一致**（不一致就重试 activate；前台丢了键就白发）
  3. `et mouse move --x (left+163) --y (top+12)`（非列表区，主菜单按光标位置分流）
  4. `et keyboard hotkey --keys ctrl+m` → 出现 `#32768` 菜单窗（`et window list --pid` 可见）
  5. **同一 bash 调用内**继续发键（菜单在 bash 调用之间会因失焦关闭）：`down×N + enter`
  6. 主菜单项序：0 新建项目▸ / 1 按名称排序 / 2 导入数据 / 3 导出数据 / 4 **设置** / 5 官方网站 / 6 退出（Down×5+Enter = 设置）
- 设置窗标题 `MLaunchSettings`（类 `MLaunchSettingsWindow`），编辑窗标题 `MLaunchItemEdit`——都可直接 `shot.ps1 -TitlePattern` 截图或 `window find`。
- 已验证流（2026-08-25 第二次会话）：Ctrl+M→菜单→设置窗打开→Enter 确认→journal `update_settings`+settings.json 落盘；Esc 取消→无 journal；Alt+1 全局热键显隐切换实测通过。

## 五、剩余工作（按 后续计划列表.md 顺序）

P0/P1 已全部收口，剩下都是 P2 与可选增强：

1. **P2**：
   - Explorer 真右键菜单（当前 kItemShellMenu 走 ShellExecuteW "properties" 属性页近似）
   - 无边框窗口阴影（复刻 frmShadow）
   - 像素级对齐：列表行高、图标灰底框去除、搜索区输入框微调（对照 `docs/poner_ui_reference.png`）
2. **P-1 尾巴**：清空分组/批量删除的"输入数量"确认框
3. **编辑窗可选增强**：Tab 焦点轮换、Ctrl+A 全选（原生 EDIT 已支持，需焦点在 EDIT 内）
4. **设置窗可选增强**：执行后隐藏开关目前是按钮切换（CCheckBoxUI 视觉态依赖图片资源未用）；热键改为按键捕获输入

## 六、本次会话（2026-08-25 第二次）改动摘要

- **设置窗 MVP**：新增 `src/ui/settings_window.{h,cpp}`（420x330 弹出窗，模式同 ItemEditWindow：Esc 取消/Enter 确认/空白拖拽/EnsureNativeEditFocused 补焦点）。
- **core 扩展**（`launcher_core.{h,cpp}`）：`SortGroupItemsByName`（大小写不敏感稳定排序+journal `sort_group`）、`ExportData`（原子写出独立快照+journal `export_data`）、`UpdateSettings`（钳制+journal `update_settings`）；SaveData 序列化抽成 `SerializeCurrentData` 复用。测试 19→22。
- **主菜单转正**：按名称排序/导出数据（`PickSaveJsonFilePath`）/设置 全部接入；非列表区域右键、Ctrl+M、Apps 键呼出菜单（`WM_KEYDOWN` 补发 `WM_CONTEXTMENU`，绕过 fork 吞 Shift+F10）。
- **全局热键**：`RegisterHotKey`（constants `kAppHotkeyId`），`ParseHotkeyString` 解析 `Ctrl+Alt+S` 式文本（字母/数字/F1-F24/命名键/OEM 键，必须带修饰键）；WM_HOTKEY 切换主窗显隐；OnClose 注销。
- **执行后隐藏**：`LaunchSelectedItem` 成功后按 `execute_hide` 隐藏主窗。
- **默认尺寸**：无 ui_state.ini 时用 settings 的窗口宽高（main.cpp `ApplyDefaultWindowSize`）与分组栏宽度（OnCreate 先 Load 后 RestoreUiState，顺序有讲究）。
- **全量中文化**：三个菜单/Toast/MessageBox/文件对话框过滤器/搜索命令行标签/回收站显示名（数据内旧英文名在显示层覆盖为"回收站"）。
- 已知小账：设置窗"执行后隐藏"开关是按钮式（开/关换色），CCheckBoxUI 视觉态依赖图片资源故未用；分组栏宽度显示原始值（86），保存时才钳制到 80-600。

## 七、快速自检清单（新会话开始时）

```powershell
cd D:\WorkSpace\mlaunch
git log --oneline -3          # 应看到本次设置窗/主菜单提交
xmake build mlaunch           # 应 build ok（先关掉在跑的 mlaunch）
xmake run core_tests          # 应 22 tests PASSED
```
UI 验证：运行 mlaunch → Ctrl+M 开主菜单 → Down×5+Enter 打开设置窗 → 改分组栏宽度 → 确定 → 检查 `%LOCALAPPDATA%\nassistant\operations.log` 出现 `update_settings` 且主窗分组栏立即变化；再按 Alt+1 验证全局热键显隐。
