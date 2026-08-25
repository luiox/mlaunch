#include <windows.h>
#include <UIlib.h>
#include <cstdio>
#include <filesystem>

#include "app_window.h"

using namespace DuiLib;

namespace {

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

} // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    SetupConsoleOutput();

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

    HWND hwnd = frame->Create(nullptr, _T("Poner"), UI_WNDSTYLE_FRAME | WS_SIZEBOX, WS_EX_WINDOWEDGE);
    if (hwnd == nullptr) {
        delete frame;
        CoUninitialize();
        return 2;
    }

    if (!frame->HasRestoredWindowPlacement()) {
        // 无已保存布局时使用设置里的默认窗口宽高。
        frame->ApplyDefaultWindowSize();
        frame->CenterWindow();
    }
    ::ShowWindow(hwnd, frame->ShouldStartMaximized() ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
    ::UpdateWindow(hwnd);

    DuiLibPaintManagerUI::MessageLoop();
    CoUninitialize();
    return 0;
}
