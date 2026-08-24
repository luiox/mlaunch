# libicon-core

`libicon-core` 是一个独立可搬迁的图标核心库。它维护“精简、可控”的 SVG 集合，并自动生成可直接被 C++ 引用的常量 API，避免业务层写死资源路径字符串。

## 设计原则
- 只保留业务必需 icon（按 `catalog/icons-100.json`）。
- 所有 SVG 扁平存放在 `icons/`。
- 通过代码生成产出 `generated/icons.h/.cpp`。
- 提供 `dynamic` / `embed` 两种接入方式。
- 对外命名空间统一为 `icon`。

## 目录结构
- `icons/`：最终使用的 SVG 目录，文件名为 `<icon_id>.svg`。
- `catalog/icons-100.json`：标准 icon 清单。
- `catalog/ant-icons.json`：`libant-qt` 迁入的通用图标清单。
- `generated/icons.h`：生成的枚举与 API 声明。
- `generated/icons.cpp`：动态/嵌入访问实现。
- `xmake.lua`：独立构建入口。

## 常用脚本
- `python scripts/generate_cpp_assets.py`
   - 根据 `icons/` + catalog 清单生成 `generated/icons.h/.cpp`。
- `python scripts/check_coverage.py`
   - 检查当前 `icons/` 覆盖率并生成缺口清单。
- `python scripts/build_minimal_feather_pack.py`
   - 从临时 Feather 源提取精简子集到 `icons/`。

## 构建（xmake）
- 手工触发生成：
   - `xmake libicon_core_codegen`
- 然后构建：
   - `xmake f -m release`
   - `xmake build libicon_core_dynamic`
   - `xmake build libicon_core_embed`

说明：默认构建不会自动重新生成 `generated/icons.h/.cpp`，避免每次 build 都失去缓存收益。

## 对外接入方式
在你的主工程里把 `libicon-core` 当子项目引入，然后：
- 动态模式：`add_deps("libicon_core_dynamic")`
- 嵌入模式：`add_deps("libicon_core_embed")`
- 头文件：`#include "icons.h"`

使用示例：
- `icon::GetDynamicPath(icon::Icon::Search)`
- `icon::GetEmbeddedSvg(icon::Icon::Search)`
- `icon::FindIcon("search")`

对于 `libant-qt` 迁入的图标，文件名统一采用 `ant_*.svg`，生成后的枚举名统一采用 `Icon::ANT_*`。

## Bootstrap（可选）
如果你要从完整 Feather zip 重新提取，可手动执行：
- `./scripts/extract_feather.ps1 -ZipPath "<your-feather-zip-path>" -TargetDir "vendor/feather"`
- 然后执行 `python scripts/build_minimal_feather_pack.py`
