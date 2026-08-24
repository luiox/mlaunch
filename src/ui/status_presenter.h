#pragma once

#include <UIlib.h>

#include <string>

/** @brief Presents transient feedback as a floating toast label that auto-hides. */
class StatusPresenter {
public:
    void Bind(DuiLib::CLabelUI* status_label, HWND host, UINT_PTR timer_id);
    void Hide();
    void Info(const std::string& text);
    void Warn(const std::string& text);
    void Error(const std::string& text);

private:
    void Show(unsigned long color, const std::string& text);

    DuiLib::CLabelUI* status_label_ = nullptr;
    HWND host_ = nullptr;
    UINT_PTR timer_id_ = 0;
};
