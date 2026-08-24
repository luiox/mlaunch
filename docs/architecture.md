# cpp-duilib-launcher 架构说明

## 1. 模块划分

- `AppWindow`：窗口生命周期、DuiLib 消息转发、UI 状态同步。
- `backend::LauncherBackend`：数据加载/迁移、分组与项目 CRUD、启动执行。
- `IconManager`：图标来源解析、主题色替换、缓存路径与顶栏图标选择。
- `StatusPresenter`：底部状态文案与颜色呈现。
- `FileIconControl`：文件图标渲染与句柄生命周期管理。
- `launcher::util::string_util`：UTF-8/Wide 字符串转换。
- `launcher::log`：关键错误与操作日志（`OutputDebugString` + 本地日志文件）。
- `launcher::constants`：命令 ID、布局阈值、定时器 ID、状态颜色。

## 2. 关键交互流程

1. 启动后 `AppWindow::OnCreate` 初始化 DuiLib 控件树。
2. `LoadBackendData` 调用 `LauncherBackend::Load` 读取 `launcher.v2.json` 与设置文件。
3. `RenderGroups` / `RenderItems` 根据后端数据渲染列表。
4. 用户交互（点击、双击、右键、拖拽）在 `Notify` 与 `HandleCustomMessage` 分支处理。
5. 写入路径统一走后端 `SaveData/SaveSettings`；UI 状态通过防抖定时器落盘。

## 3. 已完成整改点

- 统一字符串转换：移除多处重复 `Utf8ToWide/WideToUtf8` 实现。
- 常量集中管理：命令 ID/颜色/定时器统一至 `constants.h`。
- 图标解析收敛：从 `AppWindow` 中抽离为 `IconManager`。
- 消息处理修复：`WM_SIZE`/`WM_MOVE` 触发 `m_pm.NeedUpdate()` 与 UI 状态标记。
- JSON 异常恢复：`Load` 在版本不匹配或损坏时备份原文件并回退默认数据。
- 删除分组顺序：删除后合并项目保持源组顺序，并重排组 `order`。
- 错误日志：关键操作和异常写入调试输出与本地日志文件。

## 4. 第二阶段拆分（已完成）

`AppWindow` 已完成四类职责下沉，并保持原有 UI 与交互行为：

- `UiBuilder`：负责主界面与弹窗控件树构建。
- `ListController`：负责分组/项目渲染、分组切换、列表命中选择。
- `SearchController`：负责搜索模式切换、搜索 UI 同步、输入驱动刷新。
- `DialogManager`：负责分组新增/重命名对话框生命周期与确认流程。

当前 `AppWindow` 主要承担窗口生命周期、消息分发和跨控制器协调，耦合度已明显降低。