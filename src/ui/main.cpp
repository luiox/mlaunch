#include <windows.h>
#include <UIlib.h>
#include <cstdio>

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

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        return 1;
    }

    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetCurrentPath(CPaintManagerUI::GetInstancePath());
    CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

    AppWindow* frame = new AppWindow();
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
        frame->CenterWindow();
    }
    ::ShowWindow(hwnd, frame->ShouldStartMaximized() ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
    ::UpdateWindow(hwnd);

    DuiLibPaintManagerUI::MessageLoop();
    CoUninitialize();
    return 0;
}
