#pragma once

#include <Windows.h>

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
constexpr UINT kGroupClear = 1004;

constexpr UINT kItemAdd = 1101;
constexpr UINT kItemEdit = 1102;
constexpr UINT kItemDelete = 1103;
constexpr UINT kItemRunAs = 1104;
constexpr UINT kItemOpenFolder = 1105;
constexpr UINT kItemShellMenu = 1106;
constexpr UINT kItemCopyPath = 1107;
constexpr UINT kItemMoveBase = 2000;

constexpr UINT kMainNewCustom = 3001;
constexpr UINT kMainSortByName = 3002;
constexpr UINT kMainImportData = 3003;
constexpr UINT kMainExportData = 3004;
constexpr UINT kMainSettings = 3005;
constexpr UINT kMainWebSite = 3006;
constexpr UINT kMainExit = 3007;

constexpr UINT kBackupRestoreBase = 4000;
constexpr UINT kBackupRestoreMax = 9;
constexpr UINT kBackupStartFresh = 4900;
} // namespace command

/// @brief Global hotkey id used with RegisterHotKey on the main window.
constexpr int kAppHotkeyId = 0xA001;

namespace timer {
constexpr UINT_PTR kUiStateSave = 0x4E53;
constexpr UINT_PTR kListDragPoll = 0x4E54;
constexpr UINT_PTR kStatusToast = 0x4E55;
} // namespace timer

namespace layout {
constexpr int kDefaultSplitterWidth = 220;
constexpr int kMinWindowWidth = 420;
constexpr int kMinWindowHeight = 280;
constexpr int kMinGroupPanelWidth = 80;
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
