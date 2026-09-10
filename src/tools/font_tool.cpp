// 点阵字库生成工具：替代 PCtoLCD2002（2002 年 Delphi GUI，只有界面没有接口，AI 无法调用）。
// 管线：GDI 渲染字形到 1bpp 网格 → 取模引擎（逐行/逐列 × 顺/逆向 × 阴/阳码）→ C51 十六进制数组。
// 取模参数与 PCtoLCD2002 术语对齐：
//   逐行式（row_major=false→逐行）/ 逐列式；顺向=高位在前；阴码=笔迹为 1。
// 渲染用 NONANTIALIASED_QUALITY 硬阈值，与老工具的点阵观感一致。

#include "tool_registry.h"

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

#include "tool_util.h"

namespace tools {
namespace {

using internal::TextLine;

struct FontParams {
    std::string text = "中";
    int width = 16;
    int height = 16;
    std::string font_name = "SimSun"; // 宋体
    int font_px = 16;                 // em 高度（像素），宋体 12 磅 ≈ 16px@96dpi
    bool column_major = false;        // true=逐列式，false=逐行式
    bool msb_first = true;            // 顺向=高位在前（MSB）
    bool invert = false;              // true=阳码（笔迹为 0）
    bool data_only = false;           // 只输出十六进制数据行
};

// GDI 渲染单个字符到 w×h 网格，返回按行扫描的 1bpp 位图（bits[r*w+c]，1=笔迹）。
// 失败返回空（字体缺失/渲染异常），由调用方报错。
std::vector<uint8_t> RenderGlyph(wchar_t ch, int w, int h, const std::wstring& font_name,
                                 int font_px) {
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    if (mem == nullptr) {
        return {};
    }

    HBITMAP bmp = CreateBitmap(w, h, 1, 1, nullptr);
    HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(mem, bmp));
    HFONT font = CreateFontW(-font_px, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, font_name.c_str());
    HFONT old_font = static_cast<HFONT>(SelectObject(mem, font));
    if (old_bmp == nullptr || old_font == nullptr) {
        if (old_font != nullptr)
            SelectObject(mem, old_font);
        if (old_bmp != nullptr)
            SelectObject(mem, old_bmp);
        DeleteObject(font);
        DeleteObject(bmp);
        DeleteDC(mem);
        return {};
    }

    // 1bpp DDB：先铺白底再写黑字（0=黑=笔迹）。
    PatBlt(mem, 0, 0, w, h, WHITENESS);
    SetTextColor(mem, RGB(0, 0, 0));
    SetBkColor(mem, RGB(255, 255, 255));
    SetBkMode(mem, OPAQUE);
    TextOutW(mem, 0, 0, &ch, 1);

    // 读回为 top-down 1bpp DIB；每行按 DWORD 对齐（16px 宽 = 4 字节/行）。
    // DIB 位 1=白（调色板[1]），笔迹（黑）= 0，取反成"1=笔迹"。
    const int stride = ((w + 31) / 32) * 4;
    std::vector<uint8_t> dib(static_cast<std::size_t>(stride) * h, 0);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    std::vector<uint8_t> bits(static_cast<std::size_t>(w) * h, 0);
    if (GetDIBits(mem, bmp, 0, h, dib.data(), &bmi, DIB_RGB_COLORS) != 0) {
        for (int r = 0; r < h; ++r) {
            for (int c = 0; c < w; ++c) {
                const uint8_t byte = dib[static_cast<std::size_t>(r) * stride + c / 8];
                const int dib_bit = (byte >> (7 - (c % 8))) & 1; // DIB 高位在前
                bits[static_cast<std::size_t>(r) * w + c] = static_cast<uint8_t>(dib_bit == 0);
            }
        }
    }

    SelectObject(mem, old_font);
    SelectObject(mem, old_bmp);
    DeleteObject(font);
    DeleteObject(bmp);
    DeleteDC(mem);
    return bits;
}

// 取模引擎：位图 → 字节流。row-major 逐行 / 列优先逐列；MSB 顺向。
std::vector<uint8_t> ExtractBytes(const std::vector<uint8_t>& bits, int w, int h, bool column_major,
                                  bool msb_first, bool invert) {
    std::vector<uint8_t> bytes;
    auto pixel = [&](int x, int y) {
        uint8_t v = bits[static_cast<std::size_t>(y) * w + x];
        if (invert)
            v = static_cast<uint8_t>(v == 0);
        return v;
    };

    if (!column_major) {
        // 逐行式：每行 w 像素 → w/8 字节（宽需为 8 的倍数）。
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; x += 8) {
                uint8_t b = 0;
                for (int i = 0; i < 8; ++i) {
                    if (pixel(x + i, y)) {
                        b |= static_cast<uint8_t>(msb_first ? 0x80 >> i : 0x01 << i);
                    }
                }
                bytes.push_back(b);
            }
        }
    } else {
        // 逐列式：每列 h 像素 → h/8 字节（高需为 8 的倍数）。
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; y += 8) {
                uint8_t b = 0;
                for (int i = 0; i < 8; ++i) {
                    if (pixel(x, y + i)) {
                        b |= static_cast<uint8_t>(msb_first ? 0x80 >> i : 0x01 << i);
                    }
                }
                bytes.push_back(b);
            }
        }
    }
    return bytes;
}

std::string ToHexList(const std::vector<uint8_t>& bytes) {
    std::string out;
    out.reserve(bytes.size() * 6);
    char buf[8];
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0)
            out += ',';
        std::snprintf(buf, sizeof(buf), "0x%02X", bytes[i]);
        out += buf;
    }
    return out;
}

// 字符画预览：'#'=笔迹，'.'=空白。每行一条 ToolLine 由调用方拼接。
std::vector<std::string> RenderArt(const std::vector<uint8_t>& bits, int w, int h) {
    std::vector<std::string> art;
    art.reserve(h);
    for (int y = 0; y < h; ++y) {
        std::string row;
        row.reserve(w);
        for (int x = 0; x < w; ++x) {
            row += bits[static_cast<std::size_t>(y) * w + x] ? '#' : '.';
        }
        art.push_back(std::move(row));
    }
    return art;
}

bool ParseArgs(const std::string& args, FontParams& p, std::string& err) {
    std::size_t pos = 0;
    const std::string text = internal::Trim(args);
    while (pos < text.size()) {
        const std::size_t sp = text.find_first_of(" \t", pos);
        std::string tok = text.substr(pos, sp == std::string::npos ? std::string::npos : sp - pos);
        pos = sp == std::string::npos ? text.size() : sp + 1;
        if (tok.empty())
            continue;

        auto value_of = [&]() -> std::string {
            const std::size_t sp2 = text.find_first_of(" \t", pos);
            std::string v =
                text.substr(pos, sp2 == std::string::npos ? std::string::npos : sp2 - pos);
            pos = sp2 == std::string::npos ? text.size() : sp2 + 1;
            return v;
        };
        auto flag_only = [&](bool& flag) {
            flag = true;
        };

        if (tok == "--data") {
            flag_only(p.data_only);
        } else if (tok == "--column") {
            flag_only(p.column_major);
        } else if (tok == "--row") {
            p.column_major = false;
        } else if (tok == "--lsb") {
            p.msb_first = false;
        } else if (tok == "--msb") {
            p.msb_first = true;
        } else if (tok == "--invert") {
            flag_only(p.invert);
        } else if (tok == "--w") {
            p.width = std::atoi(value_of().c_str());
        } else if (tok == "--h") {
            p.height = std::atoi(value_of().c_str());
        } else if (tok == "--px") {
            p.font_px = std::atoi(value_of().c_str());
        } else if (tok == "--font") {
            p.font_name = value_of();
        } else if (tok.rfind("--", 0) == 0) {
            err = "未知参数：" + tok;
            return false;
        } else {
            // 首个非 flag 词 = 文字（支持中文，UTF-8 → UTF-16）。
            p.text = tok;
        }
    }
    if (p.width <= 0 || p.width % 8 != 0 || p.height <= 0 || p.height % 8 != 0) {
        err = "点阵宽高必须为 8 的倍数（如 16x16、24x24）";
        return false;
    }
    return true;
}

ToolOutput Run(const std::string& raw_args) {
    ToolOutput out;
    FontParams p;
    std::string err;
    if (!ParseArgs(raw_args, p, err)) {
        out.error = err;
        return out;
    }

    const std::wstring text = internal::Utf8ToWide(p.text);
    if (text.empty()) {
        out.error = "缺少文字：font <字符> [--w 16 --h 16 --font SimSun --column --invert ...]";
        return out;
    }

    // 逐字符渲染 + 取模，拼接成字库数组（多字符即批量取模）。
    std::vector<uint8_t> all_bytes;
    std::vector<std::string> art;
    for (wchar_t ch : text) {
        std::vector<uint8_t> bits =
            RenderGlyph(ch, p.width, p.height, internal::Utf8ToWide(p.font_name), p.font_px);
        if (bits.empty()) {
            out.error = "字体渲染失败：" + p.font_name;
            return out;
        }
        std::vector<uint8_t> glyph =
            ExtractBytes(bits, p.width, p.height, p.column_major, p.msb_first, p.invert);
        all_bytes.insert(all_bytes.end(), glyph.begin(), glyph.end());

        // art 存首个字符的渲染（多字符时预览第一个就够）。
        if (art.empty()) {
            art = RenderArt(bits, p.width, p.height);
        }
    }

    const std::string hex = ToHexList(all_bytes);
    out.primary = hex;

    if (p.data_only) {
        out.lines.push_back(TextLine(hex));
        out.ok = true;
        return out;
    }

    char summary[160];
    std::snprintf(summary, sizeof(summary), "\"%s\" %dx%d %s %dpx %s %s → %d 字节", p.text.c_str(),
                  p.width, p.height, p.font_name.c_str(), p.font_px,
                  p.column_major ? "逐列" : "逐行", p.msb_first ? "顺向" : "逆向",
                  static_cast<int>(all_bytes.size()));
    out.lines.push_back(TextLine(summary));

    for (const auto& row : art) {
        out.lines.push_back(TextLine(row));
    }

    // C51 风格注释头 + 数据，UI 复制给单片机工程直接可用。
    std::string c51 = "/*-- 文字: " + p.text + " --*/\r\n";
    c51 += "/*-- " + p.font_name + " " + std::to_string(p.font_px) +
           "; 宽x高=" + std::to_string(p.width) + "x" + std::to_string(p.height) + " --*/\r\n";
    c51 += hex + ";";
    ToolLine copy_line;
    copy_line.text = "回车复制 C51 格式字库数据";
    copy_line.selectable = true;
    copy_line.copy_text = c51;
    out.lines.push_back(copy_line);

    out.ok = true;
    return out;
}

} // namespace

ToolDef BuildFontTool() {
    ToolDef def;
    def.keyword = "font";
    def.name = "点阵字库生成";
    def.usage = "font <文字> [--w 16 --h 16 --font SimSun --px 16 --column --lsb --invert --data]";
    def.run = &Run;
    return def;
}

} // namespace tools
