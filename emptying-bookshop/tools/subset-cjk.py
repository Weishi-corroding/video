#!/usr/bin/env python3
"""Make fonts/NotoSansSC-Subset.otf: Noto Sans CJK SC cut down to the characters the film uses.

usage: python3 tools/subset-cjk.py [path/to/NotoSansCJK-Regular.ttc]

Run it again after adding Chinese text. It scans src/ for every non-ASCII character and keeps those
plus printable ASCII. Needs fontTools (Debian/Ubuntu: python3-fonttools, fonts-noto-cjk).
"""
import pathlib
import sys

from fontTools import subset
from fontTools.ttLib import TTCollection

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = sys.argv[1] if len(sys.argv) > 1 else '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'
OUT = ROOT / 'fonts' / 'NotoSansSC-Subset.otf'

chars = {chr(c) for c in range(0x20, 0x7F)}
for f in (ROOT / 'src').rglob('*'):
    if f.suffix in ('.cpp', '.h'):
        chars |= {ch for ch in f.read_text(encoding='utf-8') if ord(ch) > 0x7F}

face = next(font for font in TTCollection(SRC).fonts if 'CJK SC' in font['name'].getDebugName(1))
options = subset.Options()
options.layout_features = ['*']
options.name_IDs = ['*']
options.notdef_outline = True
sub = subset.Subsetter(options)
sub.populate(text=''.join(sorted(chars)))
sub.subset(face)
face.save(OUT)
print(f'{OUT.relative_to(ROOT)}: {len(chars)} characters, {OUT.stat().st_size // 1024} KB')
