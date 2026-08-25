#pragma once

#include <UIlib.h>

#include <filesystem>
#include <string>

#include "launcher_core.h"
#include "icons.h"

class IconManager {
public:
    explicit IconManager(std::filesystem::path base_dir);

    std::string ParseItemIconSource(const core::LaunchItem& item) const;
    icon::Icon ResolveTopBarIcon(icon::Icon preferred, icon::Icon fallback) const;
    DuiLib::CDuiString MakeSvgImageAttr(icon::Icon icon, int draw_px = 16, int box_px = 26) const;
    DuiLib::CDuiString MakeScrollbarThumbAttr() const;

private:
    static void ReplaceAllInPlace(std::string* text, const std::string& from, const std::string& to);
    static std::string ApplyIconThemeColor(std::string svg_text);
    static bool WriteThemedSvg(const std::filesystem::path& out_path, const std::string& raw_svg);
    std::filesystem::path BuildThemedIconPath(const std::filesystem::path& source_or_name) const;
    std::filesystem::path GetEmbeddedIconPath(icon::Icon icon) const;
    std::filesystem::path GetDynamicIconPath(icon::Icon icon) const;
    std::filesystem::path ResolveIconPath(icon::Icon icon) const;

private:
    std::filesystem::path cache_dir_;
};
