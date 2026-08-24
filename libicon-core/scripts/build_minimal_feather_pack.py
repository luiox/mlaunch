import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "catalog" / "icons-100.json"
OUT_DIR = ROOT / "icons"
MAPPING_JSON = ROOT / "catalog" / "feather-mapping.json"
REPORT_MD = ROOT / "catalog" / "minimal-feather-report.md"

source_roots = [
    ROOT / "vendor" / "feather",
    ROOT / "vendor" / "feather" / "icons",
    ROOT / "vendor" / "feather" / "feather-main" / "icons",
    ROOT / "vendor" / "feather" / "feather-master" / "icons",
]

aliases = {
    "new": ["plus"],
    "open": ["folder"],
    "delete": ["trash-2", "trash"],
    "find": ["search"],
    "replace": ["refresh-cw"],
    "back": ["arrow-left"],
    "forward": ["arrow-right"],
    "up": ["arrow-up"],
    "refresh": ["refresh-cw", "rotate-cw"],
    "zoom_in": ["zoom-in"],
    "zoom_out": ["zoom-out"],
    "fullscreen": ["maximize"],
    "fullscreen_exit": ["minimize"],
    "sort_asc": ["arrow-up"],
    "sort_desc": ["arrow-down"],
    "check": ["check"],
    "circle": ["circle"],
    "square": ["square"],
    "calendar": ["calendar"],
    "web": ["globe"],
    "external": ["external-link"],
    "prev": ["skip-back"],
    "next": ["skip-forward"],
    "volume_mute": ["volume-x"],
    "computer": ["monitor"],
    "cmd": ["terminal"],
    "shutdown": ["power"],
    "restart": ["rotate-cw", "refresh-cw"],
    "app": ["box"],
    "plus": ["plus"],
    "minus": ["minus"],
    "history": ["clock"],
    "clear": ["x-circle"],
}


def find_icon_file(stem: str) -> Path | None:
    for root in source_roots:
        if not root.exists():
            continue
        candidate = root / f"{stem}.svg"
        if candidate.exists():
            return candidate
    return None


def main() -> None:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))

    if OUT_DIR.exists():
        shutil.rmtree(OUT_DIR)
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    mapping = []
    missing = []

    for item in catalog:
        icon_id = item["id"]
        zh = item["zh"]
        candidates = [icon_id] + aliases.get(icon_id, [])

        hit_name = None
        hit_path = None
        for name in candidates:
            fp = find_icon_file(name)
            if fp is not None:
                hit_name = name
                hit_path = fp
                break

        if hit_path is None:
            missing.append({"id": icon_id, "zh": zh})
            continue

        out_file = OUT_DIR / f"{icon_id}.svg"
        shutil.copy2(hit_path, out_file)
        mapping.append(
            {
                "id": icon_id,
                "zh": zh,
                "source": "feather",
                "source_name": hit_name,
                "file": str(out_file.relative_to(ROOT)).replace("\\", "/"),
            }
        )

    MAPPING_JSON.write_text(
        json.dumps(
            {
                "total": len(catalog),
                "feather_packed": len(mapping),
                "missing": len(missing),
                "items": mapping,
                "missing_items": missing,
            },
            ensure_ascii=False,
            indent=2,
        ),
        encoding="utf-8",
    )

    lines = [
        "# Minimal Feather Pack Report",
        "",
        f"- Total required: {len(catalog)}",
        f"- Packed from Feather: {len(mapping)}",
        f"- Missing (need Remix/manual): {len(missing)}",
        "",
        "## Packed Files",
        "",
        "| ID | 中文说明 | Feather 名称 | 输出文件 |",
        "|---|---|---|---|",
    ]
    for item in mapping:
        lines.append(
            f"| `{item['id']}` | {item['zh']} | `{item['source_name']}` | `{item['file']}` |"
        )

    lines.extend(["", "## Missing", "", "| ID | 中文说明 |", "|---|---|"])
    for item in missing:
        lines.append(f"| `{item['id']}` | {item['zh']} |")

    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"generated: {MAPPING_JSON}")
    print(f"generated: {REPORT_MD}")
    print(f"packed icons: {len(mapping)} -> {OUT_DIR}")


if __name__ == "__main__":
    main()
