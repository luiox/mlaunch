import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "catalog" / "icons-100.json"
REPORT = ROOT / "catalog" / "coverage-report.md"
REMIX = ROOT / "catalog" / "remix-download-list.md"

search_roots = [
    ROOT / "icons",
    ROOT / "icons" / "remix",
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

icons = set()
for root in search_roots:
    if root.exists():
        icons |= {p.stem for p in root.glob("*.svg")}

if not icons:
    raise SystemExit(
        "No icon source found. Ensure icons exist in icons/ (and optionally icons/remix)."
    )

catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
found = []
missing = []

for item in catalog:
    icon_id = item["id"]
    candidates = [icon_id] + aliases.get(icon_id, [])
    hit = next((c for c in candidates if c in icons), None)
    if hit:
        found.append((icon_id, item["zh"], hit))
    else:
        missing.append((icon_id, item["zh"]))

coverage = (len(found) / len(catalog)) * 100 if catalog else 0

report_lines = [
    "# Feather Coverage Report",
    "",
    f"- Total: {len(catalog)}",
    f"- Covered by Feather: {len(found)}",
    f"- Missing: {len(missing)}",
    f"- Coverage: {coverage:.1f}%",
    "",
    "## Missing (need Remix/manual)",
    "",
    "| ID | 中文说明 |",
    "|---|---|",
]
for icon_id, zh in missing:
    report_lines.append(f"| `{icon_id}` | {zh} |")
REPORT.write_text("\n".join(report_lines) + "\n", encoding="utf-8")

remix_lines = [
    "# Remix Download List (Manual)",
    "",
    "Use https://remixicon.com/ search by the ID keyword below.",
    "",
    "| ID | 中文说明 | Recommended keyword |",
    "|---|---|---|",
]
for icon_id, zh in missing:
    keyword = icon_id.replace("_", "-")
    remix_lines.append(f"| `{icon_id}` | {zh} | `{keyword}` |")
REMIX.write_text("\n".join(remix_lines) + "\n", encoding="utf-8")

print(f"generated: {REPORT}")
print(f"generated: {REMIX}")
