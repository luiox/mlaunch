#include "icon_manager.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace {
constexpr const char* kThemeColor = "rgb(128,128,128)";
}

IconManager::IconManager(std::filesystem::path base_dir)
    : cache_dir_(std::move(base_dir) / "micon_cache" / "theme_128_128_128") {
    std::error_code ec;
    std::filesystem::create_directories(cache_dir_, ec);
}

std::string IconManager::ParseItemIconSource(const core::LaunchItem& item) const {
    if (!item.icon_location.empty()) {
        const auto comma = item.icon_location.find(',');
        if (comma == std::string::npos) {
            return item.icon_location;
        }
        return item.icon_location.substr(0, comma);
    }
    return item.target_path;
}

void IconManager::ReplaceAllInPlace(std::string* text, const std::string& from, const std::string& to) {
    if (text == nullptr || from.empty() || from == to) {
        return;
    }
    std::size_t start = 0;
    while ((start = text->find(from, start)) != std::string::npos) {
        text->replace(start, from.length(), to);
        start += to.length();
    }
}

std::string IconManager::ApplyIconThemeColor(std::string svg_text) {
    ReplaceAllInPlace(&svg_text, "currentColor", kThemeColor);
    ReplaceAllInPlace(&svg_text, "#000000", kThemeColor);
    ReplaceAllInPlace(&svg_text, "#000", kThemeColor);
    ReplaceAllInPlace(&svg_text, "black", kThemeColor);
    ReplaceAllInPlace(&svg_text, "rgb(0,0,0)", kThemeColor);
    return svg_text;
}

bool IconManager::WriteThemedSvg(const std::filesystem::path& out_path, const std::string& raw_svg) {
    std::error_code ec;
    std::filesystem::create_directories(out_path.parent_path(), ec);
    std::ofstream stream(out_path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }
    const std::string themed = ApplyIconThemeColor(raw_svg);
    stream.write(themed.data(), static_cast<std::streamsize>(themed.size()));
    stream.close();
    return true;
}

std::filesystem::path IconManager::BuildThemedIconPath(const std::filesystem::path& source_or_name) const {
    const auto stem = source_or_name.stem().string();
    return cache_dir_ / (stem + "_128_128_128.svg");
}

std::filesystem::path IconManager::GetDynamicIconPath(icon::Icon icon) const {
    const char* rel = icon::GetDynamicPath(icon);
    if (rel == nullptr || rel[0] == '\0') {
        return {};
    }
    const auto source = std::filesystem::current_path() / rel;
    if (!std::filesystem::exists(source)) {
        return {};
    }
    std::ifstream stream(source, std::ios::binary);
    if (!stream.is_open()) {
        return source;
    }

    const std::string raw((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    const auto out = BuildThemedIconPath(source);
    if (WriteThemedSvg(out, raw)) {
        return out;
    }
    return source;
}

std::filesystem::path IconManager::GetEmbeddedIconPath(icon::Icon icon) const {
    const icon::IconAsset* asset = icon::FindIcon(icon);
    if (asset == nullptr || asset->svg == nullptr || asset->size == 0) {
        return {};
    }

    const auto out = BuildThemedIconPath(std::filesystem::path(asset->fileName));
    if (!std::filesystem::exists(out)) {
        const std::string raw(asset->svg, asset->svg + asset->size);
        if (!WriteThemedSvg(out, raw)) {
            return {};
        }
    }
    return out;
}

std::filesystem::path IconManager::ResolveIconPath(icon::Icon icon) const {
#if MICON_ENABLE_EMBED
    return GetEmbeddedIconPath(icon);
#else
    return GetDynamicIconPath(icon);
#endif
}

icon::Icon IconManager::ResolveTopBarIcon(icon::Icon preferred, icon::Icon fallback) const {
    const auto preferred_path = ResolveIconPath(preferred);
    if (!preferred_path.empty() && std::filesystem::exists(preferred_path)) {
        return preferred;
    }
    const auto fallback_path = ResolveIconPath(fallback);
    if (!fallback_path.empty() && std::filesystem::exists(fallback_path)) {
        return fallback;
    }
    return icon::Icon::None;
}

DuiLib::CDuiString IconManager::MakeScrollbarThumbAttr() const {
    const auto path = cache_dir_ / "scroll_thumb.svg";
    std::error_code ec;
    std::filesystem::create_directories(cache_dir_, ec);
    if (!std::filesystem::exists(path)) {
        // 12x12：外圈 1px D7D7D7 描边 + 内部 EBEBEB 填充，配合 corner='1,1,1,1' 九宫格拉伸。
        static constexpr const char* kSvg =
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"12\" height=\"12\" viewBox=\"0 0 12 12\">"
            "<rect x=\"0\" y=\"0\" width=\"12\" height=\"12\" fill=\"#D7D7D7\"/>"
            "<rect x=\"1\" y=\"1\" width=\"10\" height=\"10\" fill=\"#EBEBEB\"/>"
            "</svg>";
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (stream.is_open()) {
            stream.write(kSvg, static_cast<std::streamsize>(std::char_traits<char>::length(kSvg)));
        }
    }
    if (!std::filesystem::exists(path)) {
        return {};
    }
    std::wstring path_w = path.wstring();
    std::replace(path_w.begin(), path_w.end(), L'\\', L'/');
    DuiLib::CDuiString out;
    out.Format(_T("file='%s' corner='1,1,1,1'"), path_w.c_str());
    return out;
}

DuiLib::CDuiString IconManager::MakeSvgImageAttr(icon::Icon icon, int draw_px, int box_px) const {
    if (icon == icon::Icon::None) {
        return {};
    }
    const auto path = ResolveIconPath(icon);
    if (path.empty()) {
        return {};
    }
    if (draw_px <= 0 || box_px <= 0 || draw_px > box_px) {
        draw_px = 16;
        box_px = 26;
    }

    const int offset = (box_px - draw_px) / 2;
    const int left = offset;
    const int top = offset;
    const int right = left + draw_px;
    const int bottom = top + draw_px;

    std::wstring path_w = path.wstring();
    std::replace(path_w.begin(), path_w.end(), L'\\', L'/');

    DuiLib::CDuiString out;
    out.Format(_T("file='%s' dest='%d,%d,%d,%d'"), path_w.c_str(), left, top, right, bottom);
    return out;
}
