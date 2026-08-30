#include "search_controller.h"

#include <algorithm>
#include <cctype>

#include "app_window.h"
#include "logger.h"
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
        // 此刻布局尚未执行（控件 rect 仍为 0,0,0,0），同步 SetFocus 会以
        // 0 尺寸创建原生 EDIT 且之后不再跟随重排；延迟到布局完成后聚焦。
        ::PostMessage(owner_.m_pm.GetPaintWindow(), launcher::constants::kFocusSearchMsg, 0, 0);
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

void SearchController::MoveSearchSelection(int delta) {
    if (owner_.items_list_ == nullptr || delta == 0) {
        return;
    }
    const int count = owner_.items_list_->GetCount();
    const int id_count = static_cast<int>(owner_.item_ids_.size());
    if (count <= 0 || id_count <= 0) {
        return;
    }

    // 从当前选中行向 delta 方向找下一个有效行（提示占位行 item_ids_ 为空，跳过）；
    // 未选中时 GetCurSel()==-1，向下会落到第一个有效行。
    int next = owner_.items_list_->GetCurSel();
    while (true) {
        next += delta;
        if (next < 0 || next >= count) {
            return; // 不环绕：到头/尾即停。
        }
        if (next < id_count && !owner_.item_ids_[next].empty()) {
            break;
        }
    }
    owner_.SelectItemByIndex(next);
}
