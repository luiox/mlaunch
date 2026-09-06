// 正弦波数据生成工具：替代 D:\资料\工具软件\7_DAC实验涉及工具\正弦波数据生成工具（35MB GUI exe）。
// 公式：y = floor(A + A*sin(2π*f0*k/fs + φ))，钳位 [0, 2A-1]（A 为 DAC 半量程）。
// 与老工具 sin_config.txt(A=2048,f0=10,fs=500,phi=0,t=1) 的 sin_data.txt 逐点对比：
// 497/500 位级一致，3 处过零点相差 1 LSB（老工具浮点实现指纹，对波形无意义）。
// --config 可直接读老工具的 sin_config.txt，迁移零成本。

#include "tool_registry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "tool_util.h"

namespace tools {
namespace {

using internal::TextLine;

struct SineParams {
    double amp = 2048.0;    // A：DAC 半量程，输出范围 [0, 2A-1]
    double f0 = 10.0;       // 信号频率 Hz
    double fs = 500.0;      // 采样率 Hz
    double phi_deg = 0.0;   // 初始相位（度）
    double t_sec = 1.0;     // 时长（秒），总点数 = fs * t
    int points = 0;         // 直接指定点数（覆盖 fs*t；0 = 不指定）
};

// 核心纯函数：生成采样序列。
std::vector<int> Generate(const SineParams& p) {
    const int n = p.points > 0 ? p.points : static_cast<int>(p.fs * p.t_sec);
    const double amp = p.amp;
    const int hi = static_cast<int>(2 * amp) - 1;
    std::vector<int> out;
    out.reserve(n);
    for (int k = 0; k < n; ++k) {
        const double phase = 2.0 * 3.14159265358979323846 * p.f0 * k / p.fs +
                             p.phi_deg * 3.14159265358979323846 / 180.0;
        int v = static_cast<int>(std::floor(amp + amp * std::sin(phase)));
        if (v < 0) v = 0;
        if (v > hi) v = hi;
        out.push_back(v);
    }
    return out;
}

// 逗号分隔单行（对齐老工具 sin_data.txt 的输出格式）。
std::string JoinSamples(const std::vector<int>& samples) {
    std::string out;
    out.reserve(samples.size() * 5);
    char buf[16];
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (i > 0) out += ',';
        std::snprintf(buf, sizeof(buf), "%d", samples[i]);
        out += buf;
    }
    return out;
}

// ASCII 包络图（示波器风格）：每列画采样 min..max 的竖线。
std::vector<std::string> RenderArt(const std::vector<int>& samples, double amp,
                                   int cols = 64, int rows = 12) {
    std::vector<std::string> art(rows, std::string(cols, ' '));
    const double lo = 0.0;
    const double hi = 2.0 * amp;
    const double per_col = static_cast<double>(samples.size()) / cols;
    for (int c = 0; c < cols; ++c) {
        const int b0 = static_cast<int>(c * per_col);
        const int b1 = std::min(static_cast<int>((c + 1) * per_col), static_cast<int>(samples.size()));
        if (b0 >= b1) continue;
        int mn = samples[b0];
        int mx = samples[b0];
        for (int i = b0 + 1; i < b1; ++i) {
            mn = std::min(mn, samples[i]);
            mx = std::max(mx, samples[i]);
        }
        auto to_row = [&](double v) {
            int r = rows - 1 - static_cast<int>((v - lo) / (hi - lo + 1e-9) * rows);
            return std::clamp(r, 0, rows - 1);
        };
        const int r0 = to_row(mn);
        const int r1 = to_row(mx);
        for (int r = std::min(r0, r1); r <= std::max(r0, r1); ++r) {
            art[r][c] = '*';
        }
    }
    return art;
}

// 解析 --flag value 形式参数（数值 flag 直接覆盖）。
bool ParseArgs(const std::string& args, SineParams& p, bool& data_only,
               std::string& config_err) {
    std::size_t pos = 0;
    data_only = false;
    const std::string text = internal::Trim(args);
    while (pos < text.size()) {
        const std::size_t sp = text.find_first_of(" \t", pos);
        std::string tok = text.substr(pos, sp == std::string::npos ? std::string::npos : sp - pos);
        pos = sp == std::string::npos ? text.size() : sp + 1;
        if (tok.empty()) continue;

        auto value_of = [&]() -> std::string {
            const std::size_t sp2 = text.find_first_of(" \t", pos);
            std::string v = text.substr(pos, sp2 == std::string::npos ? std::string::npos : sp2 - pos);
            pos = sp2 == std::string::npos ? text.size() : sp2 + 1;
            return v;
        };

        if (tok == "--data") {
            data_only = true;
        } else if (tok == "--amp") {
            p.amp = std::atof(value_of().c_str());
        } else if (tok == "--f0") {
            p.f0 = std::atof(value_of().c_str());
        } else if (tok == "--fs") {
            p.fs = std::atof(value_of().c_str());
        } else if (tok == "--phi") {
            p.phi_deg = std::atof(value_of().c_str());
        } else if (tok == "--t") {
            p.t_sec = std::atof(value_of().c_str());
        } else if (tok == "--points") {
            p.points = std::atoi(value_of().c_str());
        } else if (tok == "--config") {
            // 兼容老工具 sin_config.txt：每行 "key = value"，忽略 [section]。
            const std::string path = value_of();
            std::ifstream in(std::filesystem::path(internal::Utf8ToWide(path)));
            if (!in) {
                config_err = "无法打开配置文件：" + path;
                return false;
            }
            std::string line;
            while (std::getline(in, line)) {
                const auto eq = line.find('=');
                if (eq == std::string::npos) continue;
                const std::string key = internal::Trim(line.substr(0, eq));
                const std::string val = internal::Trim(line.substr(eq + 1));
                if (key == "A") p.amp = std::atof(val.c_str());
                else if (key == "f0") p.f0 = std::atof(val.c_str());
                else if (key == "fs") p.fs = std::atof(val.c_str());
                else if (key == "phi") p.phi_deg = std::atof(val.c_str());
                else if (key == "t") p.t_sec = std::atof(val.c_str());
            }
        } else {
            config_err = "未知参数：" + tok + "（支持 --amp/--f0/--fs/--phi/--t/--points/--config/--data）";
            return false;
        }
    }
    return true;
}

ToolOutput Run(const std::string& raw_args) {
    ToolOutput out;
    SineParams p;
    bool data_only = false;
    std::string err;
    if (!ParseArgs(raw_args, p, data_only, err)) {
        out.error = err;
        return out;
    }
    if (p.amp <= 0 || p.fs <= 0 || p.f0 < 0 || p.t_sec < 0) {
        out.error = "参数非法：amp/fs 必须为正";
        return out;
    }

    const std::vector<int> samples = Generate(p);
    const std::string data = JoinSamples(samples);
    out.primary = data;

    if (data_only) {
        out.lines.push_back(TextLine(data));
        out.ok = true;
        return out;
    }

    const int cycles = p.f0 > 0 && p.t_sec > 0 ? p.t_sec * p.f0 : 0;
    char summary[128];
    std::snprintf(summary, sizeof(summary),
                  "A=%.0f f0=%gHz fs=%gHz phi=%g° t=%gs → %d 点 (%d 周期)",
                  p.amp, p.f0, p.fs, p.phi_deg, p.t_sec,
                  static_cast<int>(samples.size()), cycles);
    out.lines.push_back(TextLine(summary));

    for (const auto& row : RenderArt(samples, p.amp)) {
        out.lines.push_back(TextLine(row));
    }

    std::string preview = data.substr(0, 80);
    if (data.size() > 80) preview += "…";
    ToolLine copy_line;
    copy_line.text = "回车复制完整数据（" + std::to_string(data.size()) + " 字节）  " + preview;
    copy_line.selectable = true;
    copy_line.copy_text = data;
    out.lines.push_back(copy_line);

    out.ok = true;
    return out;
}

} // namespace

ToolDef BuildSineTool() {
    ToolDef def;
    def.keyword = "sine";
    def.name = "正弦波数据生成";
    def.usage = "sine [--amp A --f0 Hz --fs Hz --phi 度 --t 秒 | --config sin_config.txt] [--data]";
    def.run = &Run;
    return def;
}

} // namespace tools
