#pragma once

#include <Windows.h>

#include "launcher_core.h"

namespace launcher::constants {

namespace search_cmd {
constexpr int kNone = -1;
constexpr int kCmd = 0;
constexpr int kSettings = 1;
constexpr int kShutdown = 2;
constexpr int kReboot = 3;
constexpr int kLogoff = 4;
constexpr int kScreenoff = 5;
constexpr int kBaidu = 6;
} // namespace search_cmd

constexpr char kSearchCmdPrefix[] = "__cmd__";

namespace command {
constexpr UINT kGroupAdd = 1001;
constexpr UINT kGroupRename = 1002;
constexpr UINT kGroupDelete = 1003;

constexpr UINT kItemAdd = 1101;
constexpr UINT kItemEdit = 1102;
constexpr UINT kItemDelete = 1103;
constexpr UINT kItemRunAs = 1104;
constexpr UINT kItemOpenFolder = 1105;
constexpr UINT kItemShellMenu = 1106;
constexpr UINT kItemCopyPath = 1107;
constexpr UINT kItemToggleEnabled = 1108;
constexpr UINT kItemMoveBase = 2000;

constexpr UINT kMainNewCustom = 3001;
constexpr UINT kMainSortByName = 3002;
constexpr UINT kMainImportData = 3003;
constexpr UINT kMainExportData = 3004;
constexpr UINT kMainSettings = 3005;

constexpr UINT kMainExit = 3007;

// 新建项目的系统条目预设。
constexpr UINT kMainNewEmpty = 3008;
constexpr UINT kMainNewComputer = 3009;
constexpr UINT kMainNewControlPanel = 3010;
constexpr UINT kMainNewRecycleBin = 3011;
constexpr UINT kMainNewLogoff = 3012;
constexpr UINT kMainNewShutdown = 3013;
constexpr UINT kMainNewReboot = 3014;

// 锁定 / 自动隐藏 / 路径转换。
constexpr UINT kMainToggleLock = 3015;
constexpr UINT kMainToggleAutoHide = 3016;
constexpr UINT kMainConvertRelative = 3017;
constexpr UINT kMainConvertAbsolute = 3018;
constexpr UINT kMainSortByCount = 3019;

constexpr UINT kBackupRestoreBase = 4000;
constexpr UINT kBackupRestoreMax = 9;
constexpr UINT kBackupStartFresh = 4900;
} // namespace command

/// @brief Global hotkey id used with RegisterHotKey on the main window.
constexpr int kAppHotkeyId = 0xA001;

/// @brief 搜索模式延迟聚焦：UpdateSearchUi 时布局尚未跑（rect 仍为 0），
/// 同步 SetFocus 会以 0 尺寸创建原生 EDIT；PostMessage 到布局完成后再聚焦。
constexpr UINT kFocusSearchMsg = WM_APP + 0x1B;

/// @brief 分组对话框延迟聚焦：OpenGroupDialog 同步 SetFocus 会以旧/0 rect
/// 创建原生 EDIT 且此后不再跟随重排，输入文字与光标都不可见。
constexpr UINT kFocusGroupDialogMsg = WM_APP + 0x1C;

/// @brief 分组原地重命名编辑框的延迟聚焦（同上，等行 rect 摆好再创建原生 EDIT）。
constexpr UINT kFocusGroupRenameMsg = WM_APP + 0x1D;

namespace timer {
constexpr UINT_PTR kUiStateSave = 0x4E53;
constexpr UINT_PTR kListDragPoll = 0x4E54;
constexpr UINT_PTR kStatusToast = 0x4E55;
} // namespace timer

namespace layout {
constexpr int kDefaultSplitterWidth = 220;
constexpr int kMinWindowWidth = 320;
constexpr int kMinWindowHeight = 280;
// 分组栏宽度合法范围以 core::limits 为准（设置窗校验/钳制共用同一来源）。
constexpr int kMinGroupPanelWidth = core::limits::kGroupPanelWidthMin;
constexpr int kMaxGroupPanelWidth = core::limits::kGroupPanelWidthMax;
constexpr int kMinItemsPanelWidth = 220;
} // namespace layout

namespace color {
// Toast 深色底上的前景色（状态栏已移除，反馈走浮动 Toast）。
constexpr unsigned long kStatusInfo = 0xFFFFFFFF;
constexpr unsigned long kStatusWarn = 0xFFFFC24B;
constexpr unsigned long kStatusError = 0xFFFF7A7A;
} // namespace color

constexpr UINT kUiStateSaveDelayMs = 800;
constexpr UINT kStatusToastHideMs = 2200;

} // namespace launcher::constants
