#!/usr/bin/env python3
"""Offline sanity check for data/catalog.json (schema 2). Usage: validate_catalog.py [file]"""
import json, re, sys
path = sys.argv[1] if len(sys.argv) > 1 else "data/catalog.json"
ATTR = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_'+-]*(\.[A-Za-z0-9_][A-Za-z0-9_'+-]*)*$")
errors, warnings = [], []
try:
    doc = json.load(open(path, encoding="utf-8"))
except Exception as e:
    sys.exit(f"{path}: cannot parse: {e}")
cats = {c.get("id") for c in doc.get("categories", [])}
if doc.get("schema") != 2: errors.append("schema must be 2")
seen = set()
for i, a in enumerate(doc.get("apps", [])):
    w = f"apps[{i}] ({a.get('attr')})"
    if not isinstance(a.get("attr"), str) or not ATTR.match(a["attr"]) or len(a["attr"]) > 200: errors.append(f"{w}: invalid attr"); continue
    if a["attr"] in seen: errors.append(f"{w}: duplicate attr")
    seen.add(a["attr"])
    if not a.get("name"): errors.append(f"{w}: missing name")
    if a.get("license", "free") not in ("free", "unfree"): errors.append(f"{w}: license must be free|unfree")
    for c in a.get("categories", []):
        if c not in cats: errors.append(f"{w}: unknown category {c}")
    if not a.get("categories"): warnings.append(f"{w}: no categories")
    h = a.get("homepage")
    if h and not re.match(r"^https?://\S+$", h): errors.append(f"{w}: homepage must be http(s)")
    if not a.get("description"): warnings.append(f"{w}: no description")
for m in warnings: print("warning:", m)
for m in errors: print("error:", m)
print(f"{len(seen)} apps, {len(cats)} categories, {len(errors)} errors, {len(warnings)} warnings")
sys.exit(1 if errors else 0)
