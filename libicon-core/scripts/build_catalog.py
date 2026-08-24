import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / "icon库列表.md"
OUT = ROOT / "catalog" / "icons-100.json"

line_re = re.compile(r"^\|\s*(\d+)\s*\|\s*`([^`]+)`\s*\|\s*([^|]+?)\s*\|")
heading_re = re.compile(r"^##\s+.+?、(.+?)（\d+\s*个）")

entries = []
category = ""
for raw in SOURCE.read_text(encoding="utf-8").splitlines():
    h = heading_re.match(raw.strip())
    if h:
        category = h.group(1).strip()
        continue
    m = line_re.match(raw.strip())
    if not m:
        continue
    idx = int(m.group(1))
    icon_id = m.group(2).strip()
    zh = m.group(3).strip()
    entries.append({
        "index": idx,
        "id": icon_id,
        "zh": zh,
        "category": category,
        "preferred_source": "feather-first-remix-fallback",
        "status": "todo"
    })

entries.sort(key=lambda x: x["index"])
OUT.write_text(json.dumps(entries, ensure_ascii=False, indent=2), encoding="utf-8")
print(f"generated: {OUT} ({len(entries)} icons)")
