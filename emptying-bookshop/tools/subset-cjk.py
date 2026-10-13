#!/usr/bin/env python3
"""Make the Chinese fonts in fonts/: Noto Sans CJK SC and Noto Serif CJK SC (regular and bold), cut
down to the characters the film uses.

usage: python3 tools/subset-cjk.py [font directory]   (default /usr/share/fonts/opentype/noto)

Run it again after adding Chinese text. It scans src/ for every non-ASCII character and keeps those
plus printable ASCII. Needs fontTools (Debian/Ubuntu: python3-fonttools, fonts-noto-cjk,
fonts-noto-cjk-extra for the bold and serif faces).
"""
import pathlib
import sys

from fontTools import subset
from fontTools.ttLib import TTCollection

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '/usr/share/fonts/opentype/noto')
FACES = {
    'NotoSansSC-Regular.subset.otf': 'NotoSansCJK-Regular.ttc',
    'NotoSansSC-Bold.subset.otf': 'NotoSansCJK-Bold.ttc',
    'NotoSerifSC-Regular.subset.otf': 'NotoSerifCJK-Regular.ttc',
    'NotoSerifSC-Bold.subset.otf': 'NotoSerifCJK-Bold.ttc',
}

chars = {chr(c) for c in range(0x20, 0x7F)}
for f in (ROOT / 'src').rglob('*'):
    if f.suffix in ('.cpp', '.h'):
        chars |= {ch for ch in f.read_text(encoding='utf-8') if ord(ch) > 0x7F}

for out_name, ttc in FACES.items():
    face = next(font for font in TTCollection(SRC / ttc).fonts if 'CJK SC' in font['name'].getDebugName(1))
    options = subset.Options()
    options.layout_features = ['*']
    options.name_IDs = ['*']
    options.notdef_outline = True
    sub = subset.Subsetter(options)
    sub.populate(text=''.join(sorted(chars)))
    sub.subset(face)
    out = ROOT / 'fonts' / out_name
    face.save(out)
    print(f'{out.relative_to(ROOT)}: {len(chars)} characters, {out.stat().st_size // 1024} KB')
