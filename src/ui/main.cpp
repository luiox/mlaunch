#include <windows.h>
#include <UIlib.h>
#include <cstdio>
#include <filesystem>

#include "app_window.h"

using namespace DuiLib;

namespace {

// 控制台输出仅 Debug 构建启用（xmake debug 模式定义 MLAUNCH_DEV_CONSOLE）；
// Release 以 GUI 程序运行，不能弹出控制台窗口。
#ifdef MLAUNCH_DEV_CONSOLE
void SetupConsoleOutput() {
    if (!::AttachConsole(ATTACH_PARENT_PROCESS)) {
        ::AllocConsole();
    }

    FILE* out_stream = nullptr;
    FILE* err_stream = nullptr;
    freopen_s(&out_stream, "CONOUT$", "w", stdout);
    freopen_s(&err_stream, "CONOUT$", "w", stderr);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
}
#endif

} // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // DPI 感知：per-monitor v2（文字/图标按显示器实际 DPI 原生渲染，不再
    // 系统拉伸发糊）。fork 布局度量在读取时统一 ScaleInt/ScaleRect，
    // 这里只需声明感知即可让整条管线生效。旧系统回退 per-monitor v1。
    if (::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == FALSE) {
        if (HMODULE shcore = ::LoadLibraryW(L"Shcore.dll")) {
            using SetAwarenessFn = HRESULT(WINAPI*)(int);
            if (auto set_awareness = reinterpret_cast<SetAwarenessFn>(
                    ::GetProcAddress(shcore, "SetProcessDpiAwareness"))) {
                set_awareness(2); // PROCESS_PER_MONITOR_DPI_AWARE
            }
        }
    }

    // 单实例守卫：已有实例在跑时激活它并退出（顺带避免第二实例注册热键失败）。
    // 句柄故意不关：进程退出时系统自动释放互斥体。
    HANDLE single_instance_mutex = ::CreateMutexW(nullptr, TRUE, L"Local\\mlaunch.SingleInstance");
    if (single_instance_mutex != nullptr && ::GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND existing = ::FindWindowW(L"NAssistantMainFrame", nullptr); existing != nullptr) {
            // 隐藏（自动隐藏/热键藏起）或最小化状态都还原到前台。
            ::ShowWindow(existing, SW_RESTORE);
            ::SetForegroundWindow(existing);
        }
        return 0;
    }

#ifdef MLAUNCH_DEV_CONSOLE
    SetupConsoleOutput();
#endif

    // DuiLib 的 SetCurrentPath 会把进程 CWD 改成 exe 目录，
    // 旧版 Poner 数据发现依赖启动目录，必须先在这里捕获。
    const std::filesystem::path legacy_root = std::filesystem::current_path();

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        return 1;
    }

    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetCurrentPath(CPaintManagerUI::GetInstancePath());
    CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

    AppWindow* frame = new AppWindow(legacy_root);
    if (frame == nullptr) {
        CoUninitialize();
        return 1;
    }

    HWND hwnd = frame->Create(nullptr, _T("mlaunch"), UI_WNDSTYLE_FRAME | WS_SIZEBOX, WS_EX_WINDOWEDGE);
    if (hwnd == nullptr) {
        delete frame;
        CoUninitialize();
        return 2;
    }

    // fork 初始化 PaintManager 用的是虚拟屏左上角显示器的 DPI；窗口实际落在
    // 其它缩放的显示器上时需要按真实 DPI 重建字体/布局，否则内容按逻辑尺寸渲染。
    frame->AlignDpi();

    if (!frame->HasRestoredWindowPlacement()) {
        // 无已保存布局时使用设置里的默认窗口宽高。
        frame->ApplyDefaultWindowSize();
        frame->CenterWindow();
    }
    if (frame->ShouldStartHidden()) {
        // 启动隐藏模式：不显示主窗（热键唤出）；仍走一次布局避免首次唤出错位。
        ::ShowWindow(hwnd, SW_HIDE);
    } else {
        ::ShowWindow(hwnd, frame->ShouldStartMaximized() ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
    }
    ::UpdateWindow(hwnd);

    DuiLibPaintManagerUI::MessageLoop();
    CoUninitialize();
    return 0;
}
