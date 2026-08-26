# 开发计划（待办批次）

> 2026-08-25 由用户口述记录。按批次执行，每项独立提交。

## 批次 A：搜索模式修复 ✅（2026-08-25 完成，0ebaa55）

- [x] **A1 修复搜索模式布局**：进入搜索模式后 searchBar(34px 灰条) 以叠加方式画在 body 顶部，
      body 没有被推下来（分组列第一项 Common 被盖住）。已试过显式 `root->NeedUpdate()` 无效，
      `SetFixedHeight(34)`/`SetVisible` 调用均确认执行（有日志）。需要查 CVerticalLayoutUI::SetPos
      为什么没有把 searchBar 计入（疑似 partial-update 分支或高度未生效）。
- [x] **A2 修复搜索输入框原生 EDIT 黑块**：EDIT 字高带（y≈40-58）整条纯黑。
      `nativebkcolor=0xFFD2D2D2` 已设置但未生效；对照 item_edit_window 的输入框（无 nativebkcolor、
      白底正常）。怀疑画刷路径（WM_SIZE 时序 / OCM 反射）。
- [x] **A3 空输入不显示结果**：对齐原版——搜索模式空输入时列表区不显示任何条目。
- [x] **A4 空输入显示三行居中提示**（原版文案）：
      「输入关键字开始搜索，ESC键退出搜索」「上下键选择，回车键运行」「如果你觉得 Poner 不错就介绍给朋友吧」
- [x] **A5 去掉「搜索模式：开/关」toast**（原版无）。

## 批次 B：交互对齐原版 ✅（2026-08-25 完成，04c14ca / 56b29f1）

- [x] **B1 窗口缩放去闪烁**：现在缩放过程中实时刷新导致闪烁；原版缩放结束才重排。
      方向：WM_SIZING/WM_ENTRYSIZEMOVE 期间挂起重排（LockUpdate 或延迟 NeedUpdate），
      WM_EXITSIZEMOVE 再统一重排。
- [x] **B2 删除「清空分组」右键菜单功能**：原版没有该功能，且危险、与「删除分组」重叠。
      删除范围：分组菜单项、`OpenClearGroupDialog`、确认输入逻辑、core `ClearGroup` 及其测试
      （23 个测试中对应用例同步删除）。
- [x] **B3 搜索模式下放大镜按钮保持按下态**：进入搜索模式后 🔍 持续显示 pushed/active 样式，
      退出搜索模式才恢复。IconButtonUI 需要支持"粘滞按下"状态（新增 active 状态色 + 查询搜索模式）。
- [x] **B4 ☰ 菜单弹出位置对齐原版**：现在弹到很远的左边，应贴在按钮正下方（光标位置改为按钮
      rect 下方对齐；TrackPopupMenu 用按钮 rect 底部坐标 + TPM_ 对齐标志，而不是光标位置）。
      三个菜单（☰/分组右键/条目右键）位置逻辑统一检查。
- [x] **B5 删除「官方网站」菜单项**：主菜单不再包含该项（含 ExecuteMainCommand 分支）。

## 批次 C：重构（模块化）✅（2026-08-26 完成，e8f0d69 / f86744b）

- [x] **C1 core 拆分**：`launcher_core.cpp`（约 1500 行）按职责拆为多个编译单元（类不拆）：
      持久化/备份/journal、分组与条目操作、设置、启动（ShellExecute）。
- [x] **C2 ui 拆分**：`app_window.cpp`（约 1400 行）拆出菜单与命令执行、搜索模式、
      热键/托盘/生命周期、拖拽等模块；`dialog_manager`、`list_controller`、`search_controller`
      职责边界重新梳理，为后续主题与功能扩展留出模块边界。
- [x] **C3 拆分后全量回归**：core_tests 23 个用例全绿 + 主窗/设置/编辑/分组对话框/搜索截图走查。

## 备注

- 反编译：现有 MCP 反编译工具仅支持 JVM 字节码，VB6 原生 exe 不可用；仓库内也无 .frm/.bas 源码。
  原版行为以用户描述 + `docs/poner_ui_reference.png` 截图为准。
- 新建分组已改为不弹窗直接创建「分组N」（可见分组数+1，重名自动递增），待与批次 A 一起验证提交。

## 附：搜索 bug 调查记录（批次 A 上下文）

- 复现路径：挪窗到终端遮挡区外 → 点 🔍 → 截图。黑条 + body 未下移 + 分组面板未隐藏同时出现。
- 像素证据（check_search6.png）：bar y=35-69 为 D2D2D2，其中 y≈40-58 整宽纯黑（原生 EDIT 字高带）；
  y=90 左列 E6E6E6（分组面板仍可见，Common 被黑条盖住）→ body 未重排。
- 日志证据：UpdateSearchUi mode=1、bar/panel 指针非空、SetFixedHeight(34) 已执行、
  group_panel_->SetVisible(false) 已执行——但渲染结果不变。
- 已试无效：显式 `m_pm.GetRoot()->NeedUpdate()`。
- OnPaint 分支（UIManagerWin32.cpp:460）：仅当 `m_pRoot->IsUpdateNeeded()` 才全量 `m_pRoot->SetPos`，
  否则走 `__FindControlsFromUpdate` 单控件部分更新——怀疑实际走的是部分更新分支，
  且 searchBar 的 SetPos 用了旧 rect。待查：为什么 root 标记后 `IsUpdateNeeded()` 仍为 false
  （注意 `CControlUI::NeedUpdate` 开头有 `if(!IsVisible()) return;`）。
- 原生 EDIT 黑块待查：CEditWndWin32 WM_SIZE 建刷（solid 路径）→ WM_CTLCOLOREDIT 返回 m_brush；
  OCM 反射链（UIManagerWin32::OnCtlColorEdit → SendMessage(child, OCM__BASE+WM_CTLCOLOREDIT)）看似完整。
