#include "app_window.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "app_window_internal.h"
#include "constants.h"
#include "logger.h"
#include "utils/string_util.h"

namespace {

// 指定屏幕点的有效 DPI（Shcore 动态加载，失败回退 96）。
int DpiOfPoint(POINT pt) {
    const HMONITOR monitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr) {
        return 96;
    }
    // LoadLibrary 而非 GetModuleHandle：进程未必已加载 Shcore，取句柄失败会
    // 错误地回退 96，恢复的窗口几何就会差一倍。
    if (HMODULE shcore = ::LoadLibraryW(L"Shcore.dll")) {
        using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
        if (auto get_dpi = reinterpret_cast<GetDpiForMonitorFn>(
                ::GetProcAddress(shcore, "GetDpiForMonitor"))) {
            UINT dpi_x = 96;
            UINT dpi_y = 96;
            if (SUCCEEDED(get_dpi(monitor, 0 /*MDT_EFFECTIVE_DPI*/, &dpi_x, &dpi_y)) && dpi_x > 0) {
                return static_cast<int>(dpi_x);
            }
        }
    }
    return 96;
}

} // namespace

using namespace DuiLib;

void AppWindow::RestoreUiState() {
    const auto ini_path = appwin::GetUiStatePath();

    // 1) 先完成全部读取。未保存过布局时回退到设置里的分组栏宽度。
    int splitter_width = static_cast<int>(backend_.CurrentSettings().group_panel_width);
    const bool has_splitter = group_panel_ != nullptr &&
                              appwin::ReadIniInt(ini_path, L"layout", L"splitter_width", &splitter_width);

    // 兼容两种格式：
    // 1) 老格式：left/top/right/bottom
    // 2) 新格式：left/top/width/height
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    bool has_rect = false;

    const bool has_left = appwin::ReadIniInt(ini_path, L"window", L"left", &left);
    const bool has_top = appwin::ReadIniInt(ini_path, L"window", L"top", &top);
    const bool has_width = appwin::ReadIniInt(ini_path, L"window", L"width", &width);
    const bool has_height = appwin::ReadIniInt(ini_path, L"window", L"height", &height);

    if (has_left && has_top && has_width && has_height) {
        has_rect = true;
    } else {
        int right = 0;
        int bottom = 0;
        if (appwin::ReadIniInt(ini_path, L"window", L"left", &left) &&
            appwin::ReadIniInt(ini_path, L"window", L"top", &top) &&
            appwin::ReadIniInt(ini_path, L"window", L"right", &right) &&
            appwin::ReadIniInt(ini_path, L"window", L"bottom", &bottom)) {
            width = right - left;
            height = bottom - top;
            has_rect = true;
        }
    }

    int maximized = 0;
    const bool has_maximized = appwin::ReadIniInt(ini_path, L"window", L"maximized", &maximized);

    // 2) 读取完毕立即冲刷缓存：GetPrivateProfileStringW 会在进程内缓存该
    //    INI 的句柄，不释放的话，之后 SaveUiState 的原子替换将永远失败。
    ::WritePrivateProfileStringW(nullptr, nullptr, nullptr, ini_path.wstring().c_str());

    // 3) 应用读取到的状态。窗口几何换算：ini 恒存 96 基逻辑值，
    //    DPI 感知进程看到的是物理像素，恢复时按当前窗口 DPI 放大。
    //    （分组栏宽度不换算：fork 对 FixedWidth 在读取时自行 ScaleInt。）
    // 换算 DPI 取“恢复目标位置所在显示器”：创建时 CW_USEDEFAULT 落在哪块屏
    // 并不确定，若用移动前窗口的 DPI，多显示器缩放不同时几何会差一倍。
    const POINT target_center{left + width / 2, top + height / 2};
    const int restore_dpi = DpiOfPoint(target_center);
    left = ::MulDiv(left, restore_dpi, 96);
    top = ::MulDiv(top, restore_dpi, 96);
    width = ::MulDiv(width, restore_dpi, 96);
    height = ::MulDiv(height, restore_dpi, 96);

    if (has_splitter && group_panel_ != nullptr) {
        if (splitter_width < 80) {
            splitter_width = 80;
        }
        if (splitter_width > 600) {
            splitter_width = 600;
        }
        group_panel_->SetFixedWidth(splitter_width);
    }

    if (!has_rect) {
        return;
    }
    if (width < 320 || height < 220) {
        return;
    }

    // 防离屏：恢复位置若几乎完全落在虚拟屏幕外（如更换显示器后），
    // 放弃恢复，回到默认位置，避免“打开后找不到窗口”。
    {
        RECT restored{left, top, left + width, top + height};
        RECT virtual_rect{
            ::GetSystemMetrics(SM_XVIRTUALSCREEN),
            ::GetSystemMetrics(SM_YVIRTUALSCREEN),
            ::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
            ::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN)};
        RECT intersect{};
        if (!::IntersectRect(&intersect, &restored, &virtual_rect) ||
            (intersect.right - intersect.left) < 100 ||
            (intersect.bottom - intersect.top) < 100) {
            launcher::log::Warn("ui_state window rect off-screen, ignored");
            return;
        }
    }

    // 创建后恢复窗口位置与大小，避免每次启动都回到默认尺寸。
    has_restored_window_ = true;
    ::SetWindowPos(m_hWnd, nullptr, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);

    if (has_maximized && maximized != 0) {
        start_maximized_ = true;
    }
}

void AppWindow::SaveUiState() {
    const auto ini_path = appwin::GetUiStatePath();

    appwin::UiStateSnapshot snapshot{};

    // 先从当前 UI 组装快照（内存为准），再一次性写盘。
    // 窗口几何缩回 96 基逻辑值存储（与恢复侧对称；跨 DPI 迁移时尺寸语义稳定）。
    const int save_dpi = static_cast<int>(::GetDpiForWindow(m_hWnd));
    if (group_panel_ != nullptr) {
        // GetFixedWidth 是缩放后的物理值，ui_state 统一存 96 基准逻辑值。
        int splitter_width = m_pm.GetDPIObj()->ScaleIntBack(group_panel_->GetFixedWidth());
        if (splitter_width < 80) {
            splitter_width = 80;
        }
        snapshot.splitter_width = splitter_width;
    }

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(WINDOWPLACEMENT);
    if (!GetWindowPlacement(m_hWnd, &placement)) {
        return;
    }

    // 使用 rcNormalPosition 记录“正常窗口”状态下的几何信息，
    // 这样从最大化退出后仍能恢复到用户期望的普通尺寸。
    const RECT rc = placement.rcNormalPosition;
    snapshot.left = ::MulDiv(rc.left, 96, save_dpi);
    snapshot.top = ::MulDiv(rc.top, 96, save_dpi);
    snapshot.right = ::MulDiv(rc.right, 96, save_dpi);
    snapshot.bottom = ::MulDiv(rc.bottom, 96, save_dpi);
    snapshot.width = snapshot.right - snapshot.left;
    snapshot.height = snapshot.bottom - snapshot.top;
    snapshot.maximized = placement.showCmd == SW_SHOWMAXIMIZED ? 1 : 0;

    // 事务性写入：先写临时文件，再原子替换正式配置，避免中途损坏。
    if (!appwin::WriteUiStateAtomically(ini_path, snapshot)) {
        ui_state_dirty_ = true;
        ScheduleUiStateSave();
        std::fputs("[ui_state] save failed, retry scheduled\n", stderr);
        std::fflush(stderr);
        ::OutputDebugStringA("[ui_state] save failed, retry scheduled\n");
        return;
    }

    ui_state_dirty_ = false;
}

void AppWindow::ScheduleUiStateSave() {
    if (ui_state_timer_active_) {
        ::KillTimer(m_hWnd, launcher::constants::timer::kUiStateSave);
    }
    ::SetTimer(m_hWnd, launcher::constants::timer::kUiStateSave, launcher::constants::kUiStateSaveDelayMs, nullptr);
    ui_state_timer_active_ = true;
}

void AppWindow::MarkUiStateDirty() {
    // 状态变更后采用延迟写盘，避免频繁 IO。
    ui_state_dirty_ = true;
    ScheduleUiStateSave();
}

void AppWindow::FlushUiStateIfDirty() {
    if (!ui_state_dirty_) {
        return;
    }
    SaveUiState();
}

bool AppWindow::ParseHotkeyString(const std::string& text, UINT* modifiers, UINT* virtual_key) {
    if (modifiers == nullptr || virtual_key == nullptr) {
        return false;
    }
    *modifiers = 0;
    *virtual_key = 0;

    // 按 '+' 切分；最后一个 token 是主键，其余必须是修饰键。
    std::vector<std::string> tokens;
    std::string current;
    for (const char ch : text) {
        if (ch == '+') {
            tokens.push_back(current);
            current.clear();
            continue;
        }
        if (ch != ' ' && ch != '\t') {
            current.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }
    tokens.push_back(current);
    if (tokens.size() < 2 || tokens.back().empty()) {
        // 至少一个修饰键 + 主键，避免把普通字母注册成全局热键。
        return false;
    }

    UINT mods = 0;
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        if (token == "CTRL" || token == "CONTROL") {
            mods |= MOD_CONTROL;
        } else if (token == "ALT") {
            mods |= MOD_ALT;
        } else if (token == "SHIFT") {
            mods |= MOD_SHIFT;
        } else if (token == "WIN" || token == "WINDOWS") {
            mods |= MOD_WIN;
        } else {
            return false;
        }
    }

    static const std::map<std::string, UINT> kNamedKeys = {
        {"SPACE", VK_SPACE}, {"TAB", VK_TAB}, {"ESC", VK_ESCAPE}, {"ESCAPE", VK_ESCAPE},
        {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN}, {"BACK", VK_BACK}, {"BACKSPACE", VK_BACK},
        {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT},
        {"PGUP", VK_PRIOR}, {"PGDN", VK_NEXT}, {"HOME", VK_HOME}, {"END", VK_END},
        {"INS", VK_INSERT}, {"INSERT", VK_INSERT}, {"DEL", VK_DELETE}, {"DELETE", VK_DELETE},
        {"PAUSE", VK_PAUSE}, {"CAPSLOCK", VK_CAPITAL}, {"NUMLOCK", VK_NUMLOCK},
        {"SCROLLLOCK", VK_SCROLL}, {"PRINTSCREEN", VK_SNAPSHOT},
        {"`", VK_OEM_3}, {"~", VK_OEM_3}, {"-", VK_OEM_MINUS}, {"=", VK_OEM_PLUS},
        {"[", VK_OEM_4}, {"]", VK_OEM_6}, {"\\", VK_OEM_5}, {";", VK_OEM_1},
        {"'", VK_OEM_7}, {",", VK_OEM_COMMA}, {".", VK_OEM_PERIOD}, {"/", VK_OEM_2},
    };

    const std::string key = tokens.back();
    UINT vk = 0;
    if (key.size() == 1 && key[0] >= 'A' && key[0] <= 'Z') {
        vk = key[0];
    } else if (key.size() == 1 && key[0] >= '0' && key[0] <= '9') {
        vk = key[0];
    } else if (key.size() >= 2 && key[0] == 'F' &&
               key.substr(1).find_first_not_of("0123456789") == std::string::npos) {
        const int fn = std::atoi(key.c_str() + 1);
        if (fn >= 1 && fn <= 24) {
            vk = static_cast<UINT>(VK_F1 + fn - 1);
        }
    } else if (const auto it = kNamedKeys.find(key); it != kNamedKeys.end()) {
        vk = it->second;
    }
    if (vk == 0) {
        return false;
    }

    *modifiers = mods;
    *virtual_key = vk;
    return true;
}

void AppWindow::RegisterConfiguredHotkey() {
    if (hotkey_registered_) {
        ::UnregisterHotKey(m_hWnd, launcher::constants::kAppHotkeyId);
        hotkey_registered_ = false;
    }

    const std::string hotkey_text = backend_.CurrentSettings().hotkey;
    if (hotkey_text.empty()) {
        return;
    }

    UINT mods = 0;
    UINT vk = 0;
    if (!ParseHotkeyString(hotkey_text, &mods, &vk)) {
        launcher::log::Warn("invalid hotkey text: " + hotkey_text);
        status_.Warn("全局热键格式无效：" + hotkey_text);
        return;
    }
    if (!::RegisterHotKey(m_hWnd, launcher::constants::kAppHotkeyId, mods | MOD_NOREPEAT, vk)) {
        launcher::log::Warn("register hotkey failed: " + hotkey_text + " err=" + std::to_string(::GetLastError()));
        status_.Warn("全局热键注册失败（可能被占用）：" + hotkey_text);
        return;
    }
    hotkey_registered_ = true;
}

void AppWindow::ToggleMainWindowVisibility() {
    // 最小化的窗口 IsWindowVisible 仍为真，需先按 iconic 分流，否则热键无法还原。
    if (::IsIconic(m_hWnd)) {
        ::ShowWindow(m_hWnd, SW_RESTORE);
        ::SetForegroundWindow(m_hWnd);
        return;
    }
    if (::IsWindowVisible(m_hWnd)) {
        ::ShowWindow(m_hWnd, SW_HIDE);
        return;
    }
    ::ShowWindow(m_hWnd, SW_SHOW);
    ::SetForegroundWindow(m_hWnd);
}
