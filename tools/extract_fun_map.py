#!/usr/bin/env python3
"""FUN_* -> Class::Method mapping table generator (roadmap 5.5).

Extracts FUN_XXXXXXXX addresses from comments in .cpp/.h files under code/,
pairs them with their descriptions and the files that mention them, and
writes FUN_MAP.md.

Description priority:
  1. Same-line `FUN_xxxx - description` pattern
  2. Same-line `label: `FUN_xxxx`` pattern (e.g. "Dtor: FUN_...")
  3. File/class context of the line mentioning `FUN_xxxx` (derived from the
     file name)

Usage:
    python3 tools/extract_fun_map.py
"""

import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CODE = ROOT / "code"
OUT = ROOT / "FUN_MAP.md"

FUN = r"FUN_([0-9a-f]{8})"
# 1) FUN_xxxx - description  /  FUN_xxxx description
AFTER_RE = re.compile(FUN + r"\s*(?:\u2014|–|-)?\s*([^*\n\r`]+)")
# 2) label: `FUN_xxxx`  or  label FUN_xxxx
BEFORE_RE = re.compile(r"([A-Za-z][A-Za-z0-9_:~ ]{1,40}?)\s*[: ]\s*`?(" + FUN + r")`?")
SEE_RE = re.compile(r"@see\s+(" + FUN + r")")
MIRROR_RE = re.compile(r"mirrors\s+(" + FUN + r")")
METHOD_RE = re.compile(r"\b([A-Z][A-Za-z0-9_]+)::(~?[A-Za-z0-9_]+)\b")

PUNCT_ONLY = re.compile(r"^[\s`*()\[\]/,.:;|?-]*$")


def clean(text: str) -> str:
    text = re.sub(r"^[\s`*(\[]+", "", text)
    text = re.sub(r"[\s`*)\]:;,.]+$", "", text)  # trailing punctuation only
    text = re.sub(r"\s+", " ", text)
    return text.strip()


def file_context(rel: str, text: str) -> str:
    """File context: first class/struct definition in the file, else file stem."""
    m = re.search(r"\bclass\s+(RWS::)?(C?[A-Z]\w+)", text)
    if m:
        ns = "RWS::" if (m.group(1) or "/framework/" in rel) else ""
        return f"{ns}{m.group(2)} (file: {rel})"
    stem = Path(rel).stem
    return f"{stem} (file: {rel})"


def main():
    entries = defaultdict(lambda: {"descs": set(), "files": set(), "methods": set()})

    for path in sorted(CODE.rglob("*")):
        if path.suffix not in (".cpp", ".h", ".hpp"):
            continue
        if "build" in path.parts:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        rel = path.relative_to(CODE).as_posix()

        for line in text.splitlines():
            after = {}
            for m in AFTER_RE.finditer(line):
                after[m.group(1)] = clean(m.group(2))

            for m in re.finditer(FUN, line):
                key = m.group(1)
                addr = "FUN_" + key
                e = entries[addr]
                e["files"].add(rel)

                desc = ""
                # Priority 1: label before FUN ("Ctor: FUN_...", "X -> mirrors FUN_...")
                prefix = line[: m.start()]
                prefix = re.sub(r"(mirrors|@see|see|CALL|call)\s*$", "", prefix)
                bm = re.search(r"([A-Za-z][A-Za-z0-9_:~]{1,40})\s*[:\-\s]\s*$", prefix)
                label = clean(bm.group(1)) if bm else ""
                STOP = {
                    "ref", "binary", "asm", "addr", "address", "func", "function",
                    "note", "todo", "xref", "ptr", "dat", "lab", "sub", "call",
                    "called", "mirrors", "see", "bin",
                }
                if (
                    label
                    and not label.startswith(("FUN_", "PTR_"))
                    and label.lower() not in STOP
                ):
                    desc = label
                # Priority 2: description after FUN; leading FUN_xx address is trimmed
                if not desc:
                    d = after.get(key, "")
                    d = re.sub(r"^(FUN_[0-9a-f]{8}\s*[-:]\s*)+", "", d).strip()
                    d = re.sub(r"^FUN_[0-9a-f]{8}\s*", "", d).strip()
                    d = clean(d)
                    if d and not PUNCT_ONLY.match(d):
                        desc = d
                if desc and not PUNCT_ONLY.match(desc):
                    e["descs"].add(desc)
                    for mm in METHOD_RE.finditer(desc):
                        e["methods"].add(f"{mm.group(1)}::{mm.group(2)}")

    for path in sorted(CODE.rglob("*")):
        if path.suffix not in (".cpp", ".h", ".hpp") or "build" in path.parts:
            continue
        rel = path.relative_to(CODE).as_posix()
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for rx in (SEE_RE, MIRROR_RE):
            for m in rx.finditer(text):
                addr = m.group(1)
                e = entries[addr]
                e["files"].add(rel)

    # File context as last resort (from the first class definition)
    for addr, e in entries.items():
        if not e["descs"]:
            rel = sorted(e["files"])[0]
            try:
                text = (CODE / rel).read_text(encoding="utf-8", errors="replace")
            except OSError:
                text = ""
            e["descs"].add(file_context(rel, text))

    if not entries:
        print("No FUN_ references found", file=sys.stderr)
        return 1

    def sort_key(a):
        return a[4:]

    def best_desc(e):
        if e["methods"]:
            return "; ".join(sorted(e["methods"]))
        return sorted(e["descs"], key=len)[0]

    lines = [
        "# FUN_* -> Class::Method mapping table",
        "",
        "> Phase 5.5 — generated by `python3 tools/extract_fun_map.py` (regenerable)",
        f"> Source: **{len(entries)}** distinct `FUN_*` addresses found in `code/` comments.",
        "",
        "| FUN address | Matched method / description | Source file |",
        "|---|---|---|",
    ]
    for addr in sorted(entries, key=sort_key):
        e = entries[addr]
        desc = best_desc(e).replace("|", "\\|")
        files = ", ".join(sorted(e["files"])[:3])
        if len(e["files"]) > 3:
            files += f" (+{len(e['files']) - 3})"
        lines.append(f"| `{addr}` | {desc} | `{files}` |")

    matched = sum(1 for e in entries.values() if e["methods"])
    explicit = sum(
        1 for e in entries.values()
        if e["methods"] or not any("(file:" in d for d in e["descs"])
    )
    lines += [
        "",
        "## Summary",
        "",
        f"- Total references: **{len(entries)}**",
        f"- Matched as `Class::Method`: **{matched}**",
        f"- Matched with descriptive text (total): **{explicit}**",
        f"- File context only: **{len(entries) - explicit}**",
        "",
        "*This table is generated from `FUN_*` references in code comments; it does "
        "not cover FUN addresses present in the binary but not yet wired into code "
        "(see `CC.asm`).*",
        "",
    ]
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"{OUT.name}: {len(entries)} FUN addresses, {matched} method mappings")
    return 0


if __name__ == "__main__":
    sys.exit(main())
