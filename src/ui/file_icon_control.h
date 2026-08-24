#pragma once

#include <UIlib.h>

#include <string>

class FileIconControl : public DuiLib::CControlUI {
public:
    FileIconControl() = default;
    ~FileIconControl() override;

    LPCTSTR GetClass() const override { return _T("FileIconControl"); }
    void SetIconPath(const std::wstring& path);
    void PaintStatusImage(DuiLib::UIRender* pRender) override;

private:
    void ResetIcon();
    static HICON LoadShellIcon(const std::wstring& path);

private:
    HICON icon_ = nullptr;
};
