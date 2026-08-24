import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG_FILES = [
    ROOT / "catalog" / "icons-100.json",
    ROOT / "catalog" / "ant-icons.json",
]
OUT_DIR = ROOT / "generated"
OUT_H = OUT_DIR / "icons.h"
OUT_CPP = OUT_DIR / "icons.cpp"


def read_catalog():
    merged = []
    for catalog_file in CATALOG_FILES:
        if not catalog_file.exists():
            continue
        merged.extend(json.loads(catalog_file.read_text(encoding="utf-8")))
    return merged


def find_svg(icon_id: str):
    candidates = [
        ROOT / "icons" / f"{icon_id}.svg",
    ]
    for file in candidates:
        if file.exists():
            return file
    return None


def to_pascal_case(name: str) -> str:
    if name.startswith("ant_"):
        return "ANT_" + to_pascal_case(name[4:])

    parts = [p for p in name.replace("-", "_").split("_") if p]
    if not parts:
        return "Unknown"
    return "".join(p[:1].upper() + p[1:] for p in parts)


def escape_c_string(text: str) -> str:
    text = text.replace("\\", "\\\\")
    text = text.replace('"', '\\"')
    text = text.replace("\r", "")
    text = text.replace("\n", "\\n\n")
    return text


def build():
    catalog = read_catalog()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    resolved = []
    used_enum = set()
    for item in catalog:
        icon_id = item["id"]
        enum_name = to_pascal_case(icon_id)
        while enum_name in used_enum:
            enum_name += "_"
        used_enum.add(enum_name)

        file = find_svg(icon_id)
        svg_text = None
        if file is not None:
            svg_text = file.read_text(encoding="utf-8")

        resolved.append(
            {
                "id": icon_id,
                "enum": enum_name,
                "svg": svg_text,
            }
        )

    header = []
    header.append("#pragma once")
    header.append("")
    header.append("#include <cstddef>")
    header.append("#include <cstdint>")
    header.append("")
    header.append("namespace icon {")
    header.append("")
    header.append("enum class Icon : uint32_t {")
    header.append("    None = 0,")
    for row in resolved:
        header.append(f"    {row['enum']},")
    header.append("    _Count")
    header.append("};")
    header.append("")
    header.append("struct IconAsset {")
    header.append("    Icon icon;")
    header.append("    const char* id;")
    header.append("    const char* fileName;")
    header.append("    const char* svg;")
    header.append("    std::size_t size;")
    header.append("};")
    header.append("")
    header.append("std::size_t GetIconCount();")
    header.append("const IconAsset* GetAllIcons();")
    header.append("const IconAsset* FindIcon(Icon icon);")
    header.append("const IconAsset* FindIcon(const char* id);")
    header.append("")
    header.append("const char* GetDynamicPath(Icon icon);")
    header.append("const char* GetDynamicPath(const char* id);")
    header.append("")
    header.append("const char* GetEmbeddedSvg(Icon icon);")
    header.append("const char* GetEmbeddedSvg(const char* id);")
    header.append("")
    header.append("} // namespace icon")

    cpp = []
    cpp.append('#include "icons.h"')
    cpp.append("")
    cpp.append("#include <cstring>")
    cpp.append("")
    cpp.append("namespace icon {")
    cpp.append("")

    cpp.append("#if LIBICON_CORE_ENABLE_EMBED")
    for row in resolved:
        if row["svg"] is None:
            continue
        escaped = escape_c_string(row["svg"])
        cpp.append(f"static const char kSvg_{row['enum']}[] = \"{escaped}\";")
    cpp.append("#endif")
    cpp.append("")

    cpp.append("static const IconAsset kIcons[] = {")
    for row in resolved:
        file_name = f"icons/{row['id']}.svg"
        if row["svg"] is None:
            cpp.append(
                f'    {{Icon::{row["enum"]}, "{row["id"]}", "{file_name}", nullptr, 0}},'
            )
        else:
            size = len(row["svg"].encode("utf-8"))
            cpp.append("#if LIBICON_CORE_ENABLE_EMBED")
            cpp.append(
                f'    {{Icon::{row["enum"]}, "{row["id"]}", "{file_name}", kSvg_{row["enum"]}, {size}}},'
            )
            cpp.append("#else")
            cpp.append(
                f'    {{Icon::{row["enum"]}, "{row["id"]}", "{file_name}", nullptr, 0}},'
            )
            cpp.append("#endif")
    cpp.append("};")
    cpp.append("")

    cpp.append("std::size_t GetIconCount() {")
    cpp.append("    return sizeof(kIcons) / sizeof(kIcons[0]);")
    cpp.append("}")
    cpp.append("")
    cpp.append("const IconAsset* GetAllIcons() {")
    cpp.append("    return kIcons;")
    cpp.append("}")
    cpp.append("")
    cpp.append("const IconAsset* FindIcon(Icon icon) {")
    cpp.append("    if (icon == Icon::None) {")
    cpp.append("        return nullptr;")
    cpp.append("    }")
    cpp.append("    for (const auto& item : kIcons) {")
    cpp.append("        if (item.icon == icon) {")
    cpp.append("            return &item;")
    cpp.append("        }")
    cpp.append("    }")
    cpp.append("    return nullptr;")
    cpp.append("}")
    cpp.append("")
    cpp.append("const IconAsset* FindIcon(const char* id) {")
    cpp.append("    if (id == nullptr) {")
    cpp.append("        return nullptr;")
    cpp.append("    }")
    cpp.append("    for (const auto& item : kIcons) {")
    cpp.append("        if (std::strcmp(item.id, id) == 0) {")
    cpp.append("            return &item;")
    cpp.append("        }")
    cpp.append("    }")
    cpp.append("    return nullptr;")
    cpp.append("}")
    cpp.append("")
    cpp.append("const char* GetDynamicPath(Icon icon) {")
    cpp.append("    const auto* item = FindIcon(icon);")
    cpp.append("    if (item == nullptr) {")
    cpp.append("        return nullptr;")
    cpp.append("    }")
    cpp.append("    return item->fileName;")
    cpp.append("}")
    cpp.append("")
    cpp.append("const char* GetDynamicPath(const char* id) {")
    cpp.append("    const auto* item = FindIcon(id);")
    cpp.append("    if (item == nullptr) {")
    cpp.append("        return nullptr;")
    cpp.append("    }")
    cpp.append("    return item->fileName;")
    cpp.append("}")
    cpp.append("")
    cpp.append("const char* GetEmbeddedSvg(Icon icon) {")
    cpp.append("    const auto* item = FindIcon(icon);")
    cpp.append("    if (item == nullptr) {")
    cpp.append("        return nullptr;")
    cpp.append("    }")
    cpp.append("    return item->svg;")
    cpp.append("}")
    cpp.append("")
    cpp.append("const char* GetEmbeddedSvg(const char* id) {")
    cpp.append("    const auto* item = FindIcon(id);")
    cpp.append("    if (item == nullptr) {")
    cpp.append("        return nullptr;")
    cpp.append("    }")
    cpp.append("    return item->svg;")
    cpp.append("}")
    cpp.append("")
    cpp.append("} // namespace icon")

    OUT_H.write_text("\n".join(header) + "\n", encoding="utf-8")
    OUT_CPP.write_text("\n".join(cpp) + "\n", encoding="utf-8")
    print(f"generated: {OUT_H}")
    print(f"generated: {OUT_CPP}")


if __name__ == "__main__":
    build()
