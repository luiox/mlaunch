#include "search_controller.h"

#include <algorithm>
#include <cctype>

#include "app_window.h"
#include "utils/string_util.h"

SearchController::SearchController(AppWindow& owner)
    : owner_(owner) {}

bool SearchController::IsSearchMode() const {
    return owner_.search_mode_;
}

void SearchController::UpdateSearchUi() {
    if (owner_.search_bar_ != nullptr) {
        owner_.search_bar_->SetVisible(owner_.search_mode_);
        owner_.search_bar_->SetFixedHeight(owner_.search_mode_ ? 34 : 0);
    }
    if (owner_.group_panel_ != nullptr) {
        owner_.group_panel_->SetVisible(!owner_.search_mode_);
    }
    if (owner_.panel_splitter_ != nullptr) {
        owner_.panel_splitter_->SetVisible(!owner_.search_mode_);
    }
    if (owner_.search_input_ != nullptr) {
        owner_.search_input_->SetVisible(owner_.search_mode_);
    }
    // 放大镜按钮粘滞按下态：搜索模式期间恒显示 pushed 底色。
    if (owner_.search_button_ != nullptr) {
        owner_.search_button_->SetActive(owner_.search_mode_);
    }
    // 此 fork OnPaint 只在 root 自身被标记时才全量重排（否则走单控件部分更新，
    // 会在旧位置叠加绘制），显式标记 root 强制整窗重排。
    if (owner_.m_pm.GetRoot() != nullptr) {
        owner_.m_pm.GetRoot()->NeedUpdate();
    }
    owner_.m_pm.NeedUpdate();
    if (owner_.search_mode_ && owner_.search_input_ != nullptr) {
        owner_.search_input_->SetFocus();
        const int text_len = owner_.search_input_->GetText().GetLength();
        owner_.search_input_->SetSel(text_len, text_len);
    }
}

void SearchController::ToggleSearchMode() {
    owner_.search_mode_ = !owner_.search_mode_;
    if (!owner_.search_mode_ && owner_.search_input_ != nullptr) {
        owner_.search_input_->SetText(_T(""));
        active_command_ = launcher::constants::search_cmd::kNone;
        baidu_keyword_.clear();
    }
    UpdateSearchUi();
    owner_.RenderItems();
    // 对齐原版：切换搜索模式不弹 toast。
}

std::string SearchController::CommandIdToItemId(int cmd_id) {
    return launcher::constants::kSearchCmdPrefix + std::to_string(cmd_id);
}

int SearchController::ParseCommand(const std::string& input, std::string* out_keyword) {
    std::string trimmed = input;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), trimmed.end());

    auto to_lower = [](const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return result;
    };

    const std::string lower = to_lower(trimmed);

    if (lower == "cmd" || lower.rfind("cmd ", 0) == 0) {
        return launcher::constants::search_cmd::kCmd;
    }
    if (lower == "setting" || lower == "settings" ||
        lower.rfind("setting ", 0) == 0 || lower.rfind("settings ", 0) == 0) {
        return launcher::constants::search_cmd::kSettings;
    }
    if (lower == "shutdown" || lower.rfind("shutdown ", 0) == 0) {
        return launcher::constants::search_cmd::kShutdown;
    }
    if (lower == "reboot" || lower.rfind("reboot ", 0) == 0) {
        return launcher::constants::search_cmd::kReboot;
    }
    if (lower == "logoff" || lower.rfind("logoff ", 0) == 0) {
        return launcher::constants::search_cmd::kLogoff;
    }
    if (lower == "screenoff" || lower.rfind("screenoff ", 0) == 0) {
        return launcher::constants::search_cmd::kScreenoff;
    }
    if (lower.rfind("baidu ", 0) == 0) {
        if (out_keyword != nullptr) {
            *out_keyword = trimmed.substr(6);
        }
        return launcher::constants::search_cmd::kBaidu;
    }

    return launcher::constants::search_cmd::kNone;
}

void SearchController::HandleInputChanged() {
    if (!owner_.search_mode_) {
        return;
    }

    std::string input;
    if (owner_.search_input_ != nullptr) {
        input = launcher::util::WideToUtf8(owner_.search_input_->GetText().GetData());
    }

    std::string keyword;
    active_command_ = ParseCommand(input, &keyword);
    baidu_keyword_ = keyword;

    owner_.RenderItems();
}
