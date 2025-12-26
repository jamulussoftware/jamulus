#!/usr/bin/env python3
import sys
import json
from pathlib import Path

if len(sys.argv) != 3:
    print("Usage: generate_vst3_manifest.py <moduleinfo.json> <output_manifest.ttl>")
    sys.exit(2)

moduleinfo = Path(sys.argv[1])
outpath = Path(sys.argv[2])
if not moduleinfo.exists():
    print(f"moduleinfo.json not found: {moduleinfo}")
    sys.exit(1)

raw = moduleinfo.read_text()
# Some helper-generated moduleinfo.json may contain trailing commas; strip them to allow parsing.
import re
raw = re.sub(r",\s*([}\]])", r"\1", raw)
j = json.loads(raw)
vendor = j.get('Factory Info', {}).get('Vendor', 'yourcompany')
version = j.get('Version', '0.1')
classes = j.get('Classes', [])

binary_name = 'JamulusVST3.vst3'

lines = []
lines.append('@prefix vst3: <http://www.steinberg.net/ns/vst3#> .')
lines.append('@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .')
lines.append('')
lines.append('<> a vst3:Factory ;')
lines.append(f'    vst3:vendor "{vendor}" ;')
lines.append(f'    vst3:version "{version}" ;')
lines.append('    vst3:component (')

for c in classes:
    cid = c.get('CID','')
    name = c.get('Name','')
    cat = c.get('Category','')
    lines.append(f'      [ vst3:cid "{cid}" ; vst3:name "{name}" ; vst3:category "{cat}" ; vst3:binary "{binary_name}" ]')

lines.append('    ) .')

outpath.write_text('\n'.join(lines), encoding='utf-8')
print(f'Wrote manifest: {outpath}')
sys.exit(0)
