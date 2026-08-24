#include "status_presenter.h"

#include "constants.h"
#include "utils/string_util.h"

void StatusPresenter::Bind(DuiLib::CLabelUI* status_label, HWND host, UINT_PTR timer_id) {
    status_label_ = status_label;
    host_ = host;
    timer_id_ = timer_id;
}

void StatusPresenter::Hide() {
    if (host_ != nullptr && timer_id_ != 0) {
        ::KillTimer(host_, timer_id_);
    }
    if (status_label_ != nullptr) {
        status_label_->SetVisible(false);
    }
}

void StatusPresenter::Show(unsigned long color, const std::string& text) {
    if (status_label_ == nullptr) {
        return;
    }
    status_label_->SetText(launcher::util::Utf8ToWide(text).c_str());
    status_label_->SetTextColor(color);
    status_label_->SetVisible(true);
    if (host_ != nullptr && timer_id_ != 0) {
        ::SetTimer(host_, timer_id_, launcher::constants::kStatusToastHideMs, nullptr);
    }
}

void StatusPresenter::Info(const std::string& text) {
    Show(launcher::constants::color::kStatusInfo, text);
}

void StatusPresenter::Warn(const std::string& text) {
    Show(launcher::constants::color::kStatusWarn, text);
}

void StatusPresenter::Error(const std::string& text) {
    Show(launcher::constants::color::kStatusError, text);
}
