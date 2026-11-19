import re
from pathlib import Path

ROOT = Path.cwd()
IDA = ROOT / "CC_ida.asm"
lines = IDA.read_text(encoding="utf-8", errors="replace").splitlines()

for cname in ['CLogic_OR', 'CWaypoint', 'CThunder']:
    print(f"\n=== {cname} ===")
    for i, line in enumerate(lines):
        if re.match(r'^sub_[0-9a-fA-F]+', line):
            if cname in line:
                print(f"  Line {i+1}: {line.strip()}")

# Also find the registration thunks
print("\n=== Registration thunks with CLogic_OR, CWaypoint, CThunder ===")
for i, line in enumerate(lines):
    if 'call sub_413700' in line:
        # look back for name/size
        name = size = ctor = None
        for j in range(i-1, max(0, i-8), -1):
            lj = lines[j]
            if name is None and re.search(r'push\s+offset\s+a[A-Za-z0-9_]+\s*;', lj):
                m = re.search(r'push\s+offset\s+(a[A-Za-z0-9_]+)\s*;', lj)
                if m:
                    name = m.group(1)
            if size is None and re.search(r'push\s+([0-9A-Fa-f]+)h?\s*;', lj):
                m = re.search(r'push\s+([0-9A-Fa-f]+)h?\s*;', lj)
                if m:
                    size = int(m.group(1).rstrip('h'), 16)
            if ctor is None and re.search(r'push\s+offset\s+sub_[0-9A-Fa-f]+', lj):
                m = re.search(r'push\s+offset\s+(sub_[0-9A-Fa-f]+)', lj)
                if m:
                    ctor = m.group(1)
        if name and (cname in name or (cname == 'CLogic_OR' and 'Logic' in name) or 
                     (cname == 'CWaypoint' and 'Waypoint' in name) or
                     (cname == 'CThunder' and 'Thunder' in name)):
            print(f"  Line {i+1}: ctor={ctor} name={name} size={size}")