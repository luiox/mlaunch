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
- **`et window find` 会匹配隐藏窗口**（visible 过滤不可靠）！判断窗口显隐必须用 `et window list --pid <pid> --include-hidden` 看 visible 字段。本次阴影窗调试时被它误导了很久（隐藏的窗口 find 依然命中）。
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

## 五、剩余工作

**计划内项目全部完成。** 可选打磨（未排期）：
- 新建分组 overlay 宽 360 > 窗口 331 时居中会贴左（低优先）。
- fork 部分更新绘制在合成条件下可能残留脏帧（见第六-a PrintWindow 坑），真实使用（鼠标 hover 持续产生失效区域）未观察到。
- 开源收尾：仓库尚无 LICENSE，发布前需选定（如 MIT）并保留 DuiLib/libca 的上游声明。

## 五-b、设置窗打磨收尾（2026-08-26 第六次会话）：热键捕获 + 自绘复选框

- **热键捕获**：热键框改只读，点击进入捕获模式录制组合键（词表与 `ParseHotkeyString` 逐项核对一致）。Esc 取消 / Tab 取消并轮换 / 仅按修饰键显示进度 / 无修饰键提示拒绝。全流程 et 自动化实测通过，含真实按键的全局热键闭环（捕获 Alt+1 → Enter → 物理按下主窗显隐切换）。
- **自绘 `appui::CheckBoxUI`**（ui_controls）：14px 边框盒+蓝底白勾，替换"执行后最小化"按钮式开关，规避 fork CCheckBoxUI 依赖图片资源的问题。
- **自动化走查揪出三个真 bug（均已修）**：
  1. **Tab 在捕获态收不到**：fork 的 MessageLoop 在派发前用 `PreMessageHandler` 吞掉 VK_TAB 做 SetNextTabControl（KNOWN_ISSUES #10 的真正机制在**消息循环层**，比文档写的更深）。修法：SettingsWindow 实现 fork 的 `ITranslateAccelerator` 并 AddTranslateAccelerator——这是唯一早于 PreMessageHandler 的扩展点。
  2. **Tab 轮换后原生 EDIT 持焦拦截按键**：点击热键框进捕获后，宽度框的 EDIT 子窗口仍持有键盘焦点，KEYDOWN 全进子窗口。修法：StartHotkeyCapture 里 `::SetFocus(m_hWnd)` 收归顶层；同时废掉 kFocusEditMsg 对只读热键框的自动聚焦（WM_ACTIVATE 每次触发都会创建寄生 EDIT，把 Esc/点击也拦住）。
  3. **CheckBoxUI::Activate 先通知后翻转**：ButtonUI::Activate 同步发 CLICK 通知时 IsChecked 还是旧值，宿主保存拿到旧状态。修法与其它字段对齐：Confirm() 统一读控件状态，CLICK 分支不写 draft。
- 自动化坑（新）：**后台应用会抢前台**（calibre 定时任务弹窗），et 发键前必须 activate→校验 fg→再发，必要时重试一次；PrintWindow 对刚失焦再激活的窗可能给陈旧帧，拿不准就用整屏 CopyFromScreen 对照。

## 五-c、开源/合流状态（2026-08-26 第六次会话末）

- `feature/dev-plan-abc` 已 fast-forward 合入 **main** 并推送（commit 至 ci 修正共 13 个），分支已删。**main 即最终态，可直接开源**；尚缺 LICENSE（需用户选定，建议 MIT）。
- 新增 `.github/workflows/build.yml`（windows release/debug 矩阵 + core_tests + exe 产物），**已全绿**（run 33023183400）。排障记录：GitHub 上 xmake 的 action 现名 `xmake-io/github-action-setup-xmake`（按完整 commit SHA 3a1a5dd 引用，tag 引用会触发启动失败）；`actions/checkout` 的 `submodules: true/recursive` 都会递归进嵌套 submodule，而 DuiLib fork 的 `3rd/SDL_ttf/external/` 下有 4 个无 .gitmodules 登记的死 gitlink，宿主仓库递归检出必炸——已在 duilib PR #1 里移除（ffb032c），mlaunch 的 submodule 也改为钉 fix 分支头，PR 合并后 master 即该提交。
- **DuiLib fork**（luiox/DuiLib_DuiEditor）：修复+构建两个提交经 `fix/mlaunch-backports-and-build` 走 PR review（[#1](https://github.com/luiox/DuiLib_DuiEditor/pull/1)，CI 双矩阵绿，MERGEABLE）；origin/master 已回退到合并前基点 34cf1e3，**用户点 merge 后 master 精确回到 26d4686**（mlaunch submodule 引用的正是它）。第三方库提取已建 issue [#2](https://github.com/luiox/DuiLib_DuiEditor/issues/2)（含旧 refactor 分支盘点与步骤建议）；fork 的 Issues 开关已打开。

## 五-d、PR #1 审查补丁（2026-08-27 第七次会话）

- **PR #1 增补 587cd48**（已推送，submodule 已同步）：审查时实测发现两个"没改好"——
  1. **pugixml.cpp 独立 TU 与 StdAfx 冲突**：fork 的 `StdAfx.h:82` 无条件 `#define PUGIXML_HEADER_ONLY`，所有含 StdAfx.h 的 TU 内联一份 pugi 实现（inline COMDAT）；`Utils/pugixml/pugixml.cpp` 被 glob 编进来时无此宏、产出强符号 → **shared 构建链接必 LNK2005/LNK1169**（本机稳定复现），static 是冗余但侥幸能链。修法：fork 根 xmake.lua 与 mlaunch DuiLibLite 的排除清单各加一行，**不动库源码**（用户要求少改）。
  2. **shared 路径无 CI 覆盖**：README 宣称 `--kind=shared` 但 CI 只测 static。build.yml matrix 加 kind 维度（static/shared × release/debug，4 job）。shell32 等 API 经实测不加也能链（SDK 兜底），syslinks 保持原样未动。
- **KNOWN_ISSUES 增补**：#19（pugixml 冲突，已修）、#20（VS ATL 组件依赖，规避）、#10 补准机制（TAB 在 **MessageLoop 派发前**被 PreMessageHandler 拦截，宿主唯一早于它的扩展点是 `ITranslateAccelerator`+`AddTranslateAccelerator`，见五-b 第 1 条）、#5 补 bitmap 画刷路径（半透明背景色）同源黑块风险 [待修]。PR 描述同步更新（原"合并后回到 26d4686"已过时，现为 587cd48）。
- **本机构建环境两坑（血泪，务必记住）**：
  1. **VS ATL 组件**：`UIImageBoxEx.h`（经 UIlib.h 无条件包含）→ atlimage.h，VS2022 默认不装 ATL，本机全量重编必 C1083（CI 的 runner 自带 ATL 所以一直绿）。用户已装"适用于最新 v143 生成工具的 C++ ATL"（14.44 atlmfc）。
  2. **xmake 的 vstudio 探测缓存**：装完组件后 `xmake f -c`（项目级）**清不掉**，必须删 `%LOCALAPPDATA%\.xmake\cache\detect` 全局缓存重配。**不要跑 `xmake g --clean`**——会清掉全局 xmake.conf 里用户的 proxy/pkg_searchdirs 配置。
- mlaunch 本机全量重编 + core_tests 22/22 恢复通过（886870a）。

## 五-e、ESC 定性修正：库改法回退，宿主层拦截（2026-08-27 第七次会话续）

用户复核 KNOWN_ISSUES 后质疑：把"原生 EDIT 吞 ESC"当库问题是把 mlaunch 的业务
混进库了。复查**结论成立**，已按此重构（mlaunch 9c92eee / fork b10eba0 / PR #1
描述已更新）：

- **定性**：单行 EDIT 的 ESC 在 Win32 下本无默认行为，"EDIT 持焦点时键盘消息只到
  子窗口"是消息路由机制而非 bug；键位→业务动作（ESC 退出搜索模式）属宿主职责。
  库内 `SendNotify(owner, _T("escape"))` 的私有事件名会成为上游合并偏移点，也给
  其他宿主注入未知通知 → PR #1 已回退（KNOWN_ISSUES #6 改标[非库问题]）。
- **宿主解法（已实测等价）**：`AppWindow` 实现 fork 的 `ITranslateAccelerator`，
  OnCreate `m_pm.AddTranslateAccelerator(this)`、OnClose Remove（防悬垂）。
  fork 的 `CPaintManagerUI::TranslateMessage`（MessageLoop 派发前）对**子窗口消息**
  也先调它 → 可拦发往原生 EDIT 的 ESC。UI 自动化全链路验证：posted 点击放大镜 →
  EditWnd 创建 → WM_CHAR 注入 'a' → ESC 经循环到达宿主（vk=27 child=1）→ 搜索
  模式退出。
- **关键约定坑（实测踩出）**：`ITranslateAccelerator::TranslateAccelerator` 的
  返回值约定看聚合器 `CPaintManagerUI::TranslateAccelerator`：**`lResult == S_OK`
  才吞掉，`S_FALSE`=放行**（不是"非零吞掉"）。该回调对循环内**所有**消息（含鼠标）
  触发——曾有中间版本放行分支误 `return 0`（==S_OK），主窗整窗输入被吞、所有点击
  失效；SettingsWindow 原来的 S_OK/S_FALSE 本来就是对的，勿再"修正"。
- **per-monitor DPI 已落地（2026-08-31）**：mlaunch 现为 PMv2 感知进程
  （方案与陷阱全录见 `未实现功能清单.md` 结案节 + `已实现功能清单.md` 十四）。
  自动化口径变化：**et/PowerShell 等非感知工具的坐标与截图都是虚拟化坐标
  （=物理÷缩放），感知进程（ctypes 设 PMv2）才是物理像素**；跨口径对点前先声明。
  另：fork `GetInterface` 按短名匹配（"Button" 不是 "ButtonUI"），对话框
  "空白拖动 vs 按钮"判定必须走短名。
- **UI 自动化测试坑（双屏 2K + DPI）**：mlaunch 是 DPI-unaware 但 fork 布局按
  物理 2x 缩放，**posted 消息 lParam 用物理像素**（放大镜在 (1930,34)，逻辑
  (965,17) 会被 normalize 减半命中标题）；fork 原生 EDIT 窗口类名是 **`EditWnd`**
  不是 `Edit`，枚举子窗口按后者过滤会漏；EditWnd 创建依赖 OS 焦点，后台 posted
  点击不建 EDIT，需 AttachThreadInput+SetFocus 先把焦点就位；真实鼠标输入会和
  用户抢前台（NetUIHWND 等 overlay + 用户在用 Excel），posted 消息最稳。
- 测试实例已清理，mlaunch 干净构建 + core_tests 22/22 通过。
- **续（同日）：画刷补丁也是误诊，一并回退（fork b6ed066 / mlaunch 11020e8）**。
  "EDIT 缓存 CTLCOLOREDIT 句柄→用已删除句柄→黑块"机制不成立（CTLCOLOREDIT
  每次 erase 同步询问同步使用）；且 create-once 让 `SetNativeEditBkColor` 运行时
  换色永不生效。`UIEditWndWin32.cpp` 已恢复与上游零 diff。黑块根因未定位，
  KNOWN_ISSUES #5 改[待修]记录误诊结论与候选根因。**教训：PR 里剩余库源码修改
  复审后，真正必要的只有 UIManagerWin32.cpp 两处渲染修复（#1 重排整窗重绘、
  #2 LockUpdate 校验-only）**；构建面(xmake/CI/gitlink/pugixml)核验扎实保留。

## 五-a、批次 C + P1.3 收尾（2026-08-26 第五次会话）摘要

- **批次 C 拆分重构完成**：C1 `launcher_core` 拆为 persistence/launch 编译单元 + `launcher_core_internal.h`（JSON 读写/原子写/MD5/时间戳等内部共享辅助，全 inline）；C2 `app_window` 拆出 menus（菜单与命令执行）/lifecycle（ui_state+热键+显隐）+ `app_window_internal.h`。纯机械搬移，声明-定义全量核对无遗漏。e8f0d69 / f86744b。
- **C3 回归**：干净重编 + core_tests 22/22（B2 删 ClearGroup 后 23→22）+ 主窗/设置窗/编辑窗截图走查通过。
- **P1.3 搜索区视觉（本次）**：`search_bar` 灰底容器 D2D2D2 + inset 4,2,4,2，`SearchBoxUI` 改白底（对齐 VB6 Search.UserControl 灰底 + Text1 白输入区层次）。
- **搜索输入聚焦时序修复（本次）**：原 `UpdateSearchUi` 在 toggle 内同步 `SetFocus`，此刻布局未跑（rect=0,0,0,0），原生 EDIT 以 0 尺寸创建。修复：`kFocusSearchMsg`（constants.h，WM_APP+0x1B）PostMessage 延迟 + 处理器里先 `::UpdateWindow` 强制同步走 OnPaint 全量重排（PostMessage 排在 WM_PAINT 之前，仅延迟不够），再 SetFocus。实测 EDIT 落位 (12,43)-(625,60) 精确等于控件 rect 减 textpadding。
- **自动化关键坑（新，血泪）**：
  - **PrintWindow 对离屏窗口不可信**：窗口挪到屏幕外（x=-350 躲终端遮挡）后，`shot.ps1`（PrintWindow）对离屏区域渲染出黑块或陈旧像素（"Common 行变白框"实为伪影）。**验证真实视觉必须用屏幕截取**（CopyFromScreen）：窗放 x=0，左侧 0-230 条带不被终端遮挡（分组面板宽 86 恰好全在条带内），"挪到 -350 点击 → 挪回 0 截屏"是可靠流程。
  - **搜狗输入法拦截 et keyboard 单键**：字母键进入 IME 组词（弹候选窗），表现为"键发了没反应/列表没过滤"。先发一次 `shift` 切英文模式再打字。
  - **et window find 输出解析**：`ConvertFrom-Json` 取 `.data.handle` 在窗口未变时可行；主窗类名 `NAssistantMainFrame`，用 `et window list --pid <pid>` 过滤 class_name 更稳。
  - pwsh 里 rg 的 glob 参数（`src/ui/*.cpp`）不展开会报错，用 `--glob "app_window*.cpp"` 代替。

## 六、本次会话（2026-08-25 第四次）改动摘要

- **像素级对齐参考图（df138da）**：删除分组/条目区分隔线与窗外框（参考图无）；分组行高 28→33；选中条 D2D2D2 填充 + 四周 1px CDCDCD 描边（新 `GroupRowUI::DoPaint` 补画，容器级边框会被子控件盖住）+ 右缘 1px 缝隙（GroupListUI inset right=1）；条目行 childpadding 2→10；**恢复列表滚动**（CListUI 启用 vscrollbar，12px 无箭头极简样式：EBEBEB 滑块+D7D7D7 描边 SVG 九宫格 `scroll_thumb.svg`，轨道白/灰随面板；滚轮实测可用）。
- **对齐原版行为（74bf2d8）**：分组名水平居中（参考图采样证实，原为误右对齐）；顶栏三图标 MakeSvgImageAttr box_px 26→30（修正偏左上 2-3px），close dest 16→20 补偿 glyph 覆盖率（墨迹 13x14/12x10/10x10 ≈ 参考 14x14/12x8/10x10）；**启动条目后 SW_HIDE→SW_MINIMIZE**（原版最小化进任务栏），设置项改名"执行后最小化"（标签列 78→92），Alt+1 加 IsIconic 分流支持最小化态直接还原。
- **按钮状态色从未渲染的根因修复（c052c0d）**：此 fork CButtonUI **不存在** normalbkcolor/hotbkcolor/pushedbkcolor 属性（静默忽略落 CLabelUI），此前所有按钮底色/悬停反馈全部无效。新增 `appui::ButtonUI`（PaintStatusImage 按 IsHotState/IsPushedState 自绘纯色，IconButtonUI 改继承它恢复悬停灰底）+ 共享工厂 `appui::MakeTextButton`（E6E6E6/D5D5D5 高 28），主窗分组对话框/设置窗/编辑窗全部接入。编辑窗图标按钮行 24→26、右列 112→124、名称行与图标列 childvalign center。
- **关键坑（新）**：
  - **et mouse click 会点到置顶终端**（终端遮挡 x∈[230,1440]）导致焦点被抢、后续按键全进终端；菜单路由只需 `et mouse move` 定位光标（WM_CONTEXTMENU 按光标坐标 hit-test，不需要真实点击）。
  - TrackPopupMenu 是原生菜单：条目菜单序 0管理员/1位置/2资源管理器菜单/3复制路径/sep/4添加/5编辑/6删除/7移动；无初始选中时第 1 个 Down 选中第 0 项，到"编辑"需 **6 个 Down**；主菜单到"设置"5 个 Down。Enter 误触"管理员运行"会弹 `etalien.exe` 安全确认框（#32770，属 mlaunch 进程，**只能 Esc**，Enter 会真启动）。
  - `et wait element` 会段错误、`et element find` 依赖 powershell.exe（本机无）——UIA 路线不可用，原生菜单键盘导航仍是最优解。
  - 误触"按名称排序"会改真实数据顺序：恢复用 `%LOCALAPPDATA%\nassistant\backups\launcher.v2.<排序时刻>.json` 覆盖 `launcher.v2.json` 后重启。
- 验证工具沉淀：`tools/scan_icons.ps1`（顶栏图标墨迹 bbox）、`tools/scan_bands.ps1`（扫描 E6E6E6 按钮色带，验证按钮底色是否渲染）。

## 六-a、上一次会话（2026-08-25 第三次）改动摘要（d763c20 / 6699a89）

- **阴影窗（frmShadow 复刻）**：新增 `src/ui/shadow_window.{h,cpp}`——WS_EX_LAYERED|TRANSPARENT|TOOLWINDOW|NOACTIVATE 黑色剪影窗，`SetLayeredWindowAttributes` alpha 60，`SetWindowPos(shadow, main, +7,+7)` 插主窗 Z 序正下方；主窗 WM_WINDOWPOSCHANGED/WM_SHOWWINDOW 时 `Sync()`（拖动实时跟随），最小化/最大化/隐藏自动隐藏；OnCreate 里 `DwmSetWindowAttribute(DWMNCRP_DISABLED)` 关掉 DWM 软阴影保持 VB6 硬边风格。实测：+7,+7 精确跟随、移动实时跟随、Alt+1 显隐联动闭环。
- **P-1 收尾**：core `ClearGroup`（整组软删除入回收站，单次落盘，Ctrl+Z 恢复最后一条）+ 分组菜单"清空分组…"+ 数量确认对话框（输入条目数才能执行，输错拒绝，journal `clear_group`）。测试 22→23。
- **Explorer 真右键菜单**：`ShowSelectedItemShellMenu` 重写为 `SHParseDisplayName`+`SHBindToParent`+`IContextMenu`（QueryContextMenu/InvokeCommand，scratch id 0x7000-0x7FFF）；非文件系统目标退回属性页。实测打开/取消正常。
- **像素对齐**：列表行高 34→28、图标 26x26 灰底 → 20x20 透明（对齐 `poner_ui_reference.png`）。
- **弹窗键盘增强**：新增 `edit_focus_helper.{h,cpp}`——`FocusNativeEdit`（补焦点+**子类化原生 EDIT**）+ Tab 焦点轮换 + Ctrl+A 全选（WM_CHAR 0x01 与 WM_KEYDOWN 双路径，IME 开启时 'A' 变 VK_PROCESSKEY 只能靠 CHAR 路径）。Tab 进入字段全选内容（Windows 惯例）。
- **关键坑（新）**：
  - 切换输入框时必须先 `WM_CLOSE` 同步销毁旧原生 EDIT（fork 的 EDIT 失焦自毁是 PostMessage 异步，旧 EDIT 残留会让 FindWindowExW 命中错误窗口、新控件 SETFOCUS 不触发创建）。
  - **不要用 `m_pm.GetFocus()` 决定 Tab 轮换目标**：fork 的 `PreMessageHandler` 会抢先对 VK_TAB 做 `SetNextTabControl`，m_pFocus 不可信；用窗口内 `focus_index_` 成员确定性轮换。
  - 目标路径失效的条目走 Explorer 菜单 → SHParseDisplayName 失败 → 自动退回属性页（报错 toast），行为符合预期。

## 六-b、上一次会话（2026-08-25 第二次）改动摘要（fd2e2aa / d763c20 部分）

- **设置窗 MVP**：新增 `src/ui/settings_window.{h,cpp}`（420x330 弹出窗，模式同 ItemEditWindow：Esc 取消/Enter 确认/空白拖拽/补焦点）。
- **core 扩展**（`launcher_core.{h,cpp}`）：`SortGroupItemsByName`（大小写不敏感稳定排序+journal `sort_group`）、`ExportData`（原子写出独立快照+journal `export_data`）、`UpdateSettings`（钳制+journal `update_settings`）；SaveData 序列化抽成 `SerializeCurrentData` 复用。测试 19→22。
- **主菜单转正**：按名称排序/导出数据（`PickSaveJsonFilePath`）/设置 全部接入；非列表区域右键、Ctrl+M、Apps 键呼出菜单（`WM_KEYDOWN` 补发 `WM_CONTEXTMENU`，绕过 fork 吞 Shift+F10）。
- **全局热键**：`RegisterHotKey`（constants `kAppHotkeyId`），`ParseHotkeyString` 解析 `Ctrl+Alt+S` 式文本（字母/数字/F1-F24/命名键/OEM 键，必须带修饰键）；WM_HOTKEY 切换主窗显隐；OnClose 注销。
- **执行后隐藏**：`LaunchSelectedItem` 成功后按 `execute_hide` 隐藏主窗。
- **默认尺寸**：无 ui_state.ini 时用 settings 的窗口宽高（main.cpp `ApplyDefaultWindowSize`）与分组栏宽度（OnCreate 先 Load 后 RestoreUiState，顺序有讲究）。
- **全量中文化**：三个菜单/Toast/MessageBox/文件对话框过滤器/搜索命令行标签/回收站显示名（数据内旧英文名在显示层覆盖为"回收站"）。
- 已知小账：设置窗"执行后隐藏"开关是按钮式（开/关换色），CCheckBoxUI 视觉态依赖图片资源故未用；分组栏宽度显示原始值（86），保存时才钳制到 80-600。

## 六-c、本次会话（2026-08-30）改动摘要（分支 feature/vb6-feature-gaps）

- **MIT LICENSE**：已直接推 main（ec8c204）。
- **新建项目系统条目子菜单**（app_window_menus.cpp）：自定义项目…/空项目/计算机/控制面板/回收站/注销/关机/重启；
  系统条目 `shell:::{CLSID}` 目标，关机类走 `shutdown.exe` 参数；`AddPresetSystemItem` 统一落地（journal `add_item`）。
- **锁定**：`AppWindow::OnNcHitTest` 覆盖——锁定时把 HTCAPTION/HT 边缘改判 HTCLIENT；
  列表拖拽准备与分隔条拖拽加 `layout_locked_` 守卫；菜单 `MF_CHECKED`；`settings.locked` 持久化。
- **自动隐藏**：WM_ACTIVATE 失焦（WA_INACTIVE、激活方属其他线程、非最小化）时 SW_HIDE；热键恢复；`settings.autoHide` 持久化。
- **路径转换**：core `ConvertItemPaths(bool to_relative, std::string*)`（%pr%=程序目录/%cr%=盘根；
  幂等、跨盘跳过、target_path+icon_location 一并转、journal `convert_paths`、SaveData 自动备份）；
  UI 子菜单两项 + YESNO 确认。测试 22→24（含往返+幂等+边界）。
- **et 走查全部通过**：shell::: 计算机 条目经搜索过滤单击启动打开"此电脑"；锁定开→拖不动/关→可拖；
  自动隐藏开→激活他窗即隐→Alt+1 恢复→关；路径转换弹窗取消不落地（数据 MD5 不变）。
- **走查工具**：`tools/` 新增 click.ps1（SendInput 单击）/ drag.ps1（拖拽）/ region_shot.ps1 / find_icons.ps1（顶栏图标定位）/
  uia_click.ps1（UIA 枚举）/ wfp.ps1（WindowFromPoint）。经验：**et 合成单击对 DuiLib 自绘按钮不可靠（用 click.ps1）**；
  菜单导航用键盘（Ctrl+M 在顶栏空白处 → Down×N+Enter，无预高亮，Down1=第0项）；MB_YESNO 的 Esc 关不掉（无 Cancel 按钮）是系统行为。
- **发现（未解决，见未实现清单一）**：200% DPI 下窗口表面黑帧（100% 正常），输入交互不受影响；疑似 fork GDI 后端 DPI 缩放问题。
- **数据说明**：走查期间误触 VariCAD 双击启动 ×2（journal 09:32:48 两条 launch）与一次 Common 组误排序（10:06:04），
  结束后已用当日备份 `launcher.v2.20260830-093216.json` 整体恢复到当日测试前状态；journal 为 append-only 保留原始记录。
  settings.json 新增 `locked:false`/`autoHide:false` 两个键（schema 扩展，默认安全）。

## 六-d、本次会话（2026-08-31）改动摘要（分支 feature/vb6-feature-gaps，接六-c）

- **高 DPI“黑帧”结案（定性反转，重要）**：不存在用户可见渲染 bug。屏幕实拍
  （CopyFromScreen）证实 200% 下窗口完整正常；进程内一切度量均为逻辑坐标
  （GetDpiForWindow=96 / GetClientRect=482x645 / fork SetDPI(96) 不缩放 / 非 layered），
  物理窗口由系统虚拟化拉伸。“黑帧”是 **PrintWindow 伪影**：高 DPI 感知进程调
  PrintWindow 不做 DWM 虚拟化拉伸，逻辑表面 1:1 放进物理位图 → 内容只占左上角其余黑。
  修复 `tools/shot.ps1`：暗区占比 >15% 回退 CopyFromScreen。**教训：验证 DPI-unaware
  窗口的真实视觉必须屏幕实拍，PrintWindow 结果不可信。**
- **搜索键盘选择**：EDIT 持焦时方向键经 `TranslateAccelerator` 拦截转发列表
  （与 ESC 同路径复用）；`SelectItemByIndex` 统一“行号→选中态”；过滤/命令行自动
  选中首个有效行；回车启动选中项。et 实测：高亮随 Up/Down 移动、cmd 命令回车拉起 cmd.exe。
- **单实例 + 开机自启**：main.cpp CreateMutex 守卫（双开 exit 0 + 原窗前置，实测）；
  Settings.autorun + 设置窗复选框；`ApplyAutorunRegistry` 写/删 HKCU Run 键，
  OnCreate 与 ApplySettings 双点同步。实测 autorun=true 启动即写键、false 启动即删。
- **工作目录字段**：`LaunchItem.working_dir` 全链路（编辑窗“起始位置”行 + IFileDialog
  浏览、JSON workingDir、Launch→lpDirectory、占位符展开、路径转换纳入）。
- **禁用/启用 + 频率排序**：`SetItemEnabled`（幂等+journal）、`SortGroupItemsByLaunchCount`
  （降序稳定）；条目菜单开关项、主菜单“按使用频率排序”、禁用项置灰渲染；Launch 拦截禁用项。
- **ui_state err=2**：预创建目录/空文件 + flush 日志降级 Debug（首启仍可能一条缓存误报，数据完整）。
- 测试 24 → 26 全绿；新增 `tools/scan_windows.ps1`（进程窗口枚举）、`tools/uia_menu.ps1`
  （原生菜单 UIA 确定性选择）。
- **自动化新坑（血泪）**：
  1. **菜单键盘 Down 计数不稳**：本次实测两次“Down×7+Enter”都误触（一次导出对话框、
     一次锁定切换），首键疑似被菜单弹出期吞掉。**原生菜单一律用 `uia_menu.ps1`**
     （UIA InvokePattern，实测可靠；MenuItem 无 SelectionItemPattern 但有 Invoke）。
  2. **模态文件对话框会阻塞主窗消息循环**：期间对主窗的点击/截图判断全部失效；
     点击坐标若落在对话框上会误触其按钮（本次误把数据导出到 Documents\setting.json，
     已删）。对话框残留时必须先 activate 对话框句柄 + Esc 关闭再继续。
  3. **搜狗 IME 中文态吃 Enter**（组词确认）：键盘走查输入文本/回车一律用剪贴板
     ctrl+v 粘贴，不要逐键打字。
  4. **et window list / tasklist 输出经 bash 管道会被 ANSI 清屏序列搞乱**，误判
     “进程消失”；判断进程/窗口状态以 `Get-Process` / et 原始 JSON 为准。
  5. **git commit 中文信息偶发 GBK 乱码**：用 python 写 UTF-8 文件 +
     `git -c i18n.commitEncoding=utf-8 commit -F <file>`，坏消息用 --amend 修。
- **数据说明**：走查期间的 journal 新增记录（export_data 一次误触已恢复、cmd launch、
  update_settings 若干）为 append-only 审计痕迹；settings.json 新增 `autorun:false`。
  launcher.v2.json 未被修改（结束时 MD5 与会话开始一致）。

## 六-e、第二批功能会话（2026-08-31 续）

- **Explorer 菜单补全**：`ExpandItemTargetPath`（%pr%/%cr%/环境变量展开，
  与 core Launch 同规则；AppWindow 新增只读 `backend()` 访问器）；
  `ShellMenuMsgHook`（WH_MSGFILTER，TrackPopupMenu 期间把
  WM_INITMENUPOPUP/WM_MEASUREITEM/WM_DRAWITEM 转给 IContextMenu3/2——
  自绘动词与“打开方式”动态子菜单必需）。
- **滚轮调宽分组栏**：必须在 `TranslateAccelerator` 拦截——fork 的
  OnMouseWheel 在窗口过程里先把事件发给命中控件（分组列表已滚走），
  HandleCustomMessage 阶段无法撤回。锁定布局/搜索模式下禁用。
- **设置页全量扩充**：5 个新 Settings 字段（start_hidden/close_minimize/
  double_click_launch/backup_rolling_count/backup_daily_days）；
  PruneBackups 参数化；设置窗分组重构（440x566，checkbox 行带完整说明文字，
  `make_check_row`/`make_num_row`/`make_section` 工厂）；
  `ValidateRangeInput` 越界红框（#D5303A）实时校验。
- **行为接线要点**：close_minimize 只分流 closebtn 点击（菜单退出/WM_CLOSE
  不受影响）；double_click 下 ITEMCLICK 仅选中、ITEMACTIVATE 启动
  （DUI_MSGTYPE_ITEMACTIVATE）。
- **实测**：启动隐藏（startHidden=true 启动后 MainWindowHandle 无标题窗、
  Alt+1 唤出 visible=true）；关闭最小化（点击后进程存活 + IsIconic=True）；
  红框校验双向（999→736 红像素，220→0）；设置窗布局走查（四组全部字段）。
- **测试坑（新）**：备份保留测试不能靠连续保存计数——rolling 时间戳秒级，
  同秒保存同名覆盖；用预置不同日期假快照隔离验证轮转。
- 测试 26 → 28 全绿；走查后已把 startHidden/closeMinimize 还原为 false。

## 七、快速自检清单（新会话开始时）

```powershell
cd D:\WorkSpace\mlaunch
git log --oneline -3          # 应看到功能差距补全提交
xmake build mlaunch           # 应 build ok（先关掉在跑的 mlaunch）
xmake run core_tests          # 应 28 tests PASSED
```
UI 验证（菜单操作用 tools/uia_menu.ps1，勿依赖键盘 Down 计数）：
搜索：点放大镜 → 粘贴字母 → 首行高亮 → Down/Up 移动 → 回车启动选中项。
单实例：双开 mlaunch，第二个应立即退出且原窗被前置。
开机自启：设置窗勾选“开机自启”确定 → `reg query HKCU\...\Run /v mlaunch` 应有值；取消勾选后应消失。
禁用：条目右键 → 禁用条目 → 列表置灰、双击启动被拦截报错。
