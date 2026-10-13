#!/usr/bin/env python3
"""Record the narration with Piper, an offline neural text-to-speech engine.

usage: ./build/emptying-bookshop --cues | python3 tools/narrate.py [--voice MODEL.onnx] [--length-scale 1.06]

Reads "id<TAB>English line" from stdin and writes narration/<id>.wav (trimmed, 16-bit mono) and
narration/durations.tsv, which the film uses to give every line enough room. Lines whose text and voice
have not changed since the last run are kept.

Setup (once):
  python3 -m venv ~/.venvs/piper && ~/.venvs/piper/bin/pip install piper-tts
  mkdir -p ~/.cache/piper-voices && cd ~/.cache/piper-voices && ~/.venvs/piper/bin/python -m piper.download_voices en_GB-cori-high
Then run this script with ~/.venvs/piper/bin/python.
"""
import argparse
import array
import hashlib
import io
import json
import pathlib
import sys
import wave

from piper import PiperVoice, SynthesisConfig

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / 'narration'

# Respellings so the voice says names the way they are said, without changing the captions.
SAY = {
    'Zedong': 'Zuh-dong',
}


def trim(samples, rate, threshold=0.01, pad=0.06):
    """Cut leading and trailing silence, keeping a short pad."""
    limit = int(threshold * 32767)
    loud = [i for i, s in enumerate(samples) if abs(s) > limit]
    if not loud:
        return samples
    p = int(pad * rate)
    return samples[max(0, loud[0] - p):min(len(samples), loud[-1] + p)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--voice', default=str(pathlib.Path.home() / '.cache/piper-voices/en_GB-cori-high.onnx'))
    ap.add_argument('--length-scale', type=float, default=1.06, help='above 1 speaks more slowly')
    args = ap.parse_args()

    OUT.mkdir(exist_ok=True)
    manifest_path = OUT / 'manifest.json'
    manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
    voice = PiperVoice.load(args.voice)
    config = SynthesisConfig(length_scale=args.length_scale)
    durations = []
    for line in sys.stdin:
        if not line.strip():
            continue
        cue, text = line.rstrip('\n').split('\t', 1)
        spoken = text
        for word, say in SAY.items():
            spoken = spoken.replace(word, say)
        key = hashlib.sha1(f'{args.voice}|{args.length_scale}|{spoken}'.encode()).hexdigest()
        wav_path = OUT / f'{cue}.wav'
        if manifest.get(cue) != key or not wav_path.exists():
            buf = io.BytesIO()
            with wave.open(buf, 'wb') as w:
                voice.synthesize_wav(spoken, w, syn_config=config)
            buf.seek(0)
            with wave.open(buf, 'rb') as r:
                rate = r.getframerate()
                samples = array.array('h', r.readframes(r.getnframes()))
            samples = trim(samples, rate)
            with wave.open(str(wav_path), 'wb') as w:
                w.setnchannels(1)
                w.setsampwidth(2)
                w.setframerate(rate)
                w.writeframes(samples.tobytes())
            manifest[cue] = key
            print(f'{cue}: recorded', file=sys.stderr)
        with wave.open(str(wav_path), 'rb') as r:
            durations.append((cue, r.getnframes() / r.getframerate()))
    (OUT / 'durations.tsv').write_text(''.join(f'{c}\t{d:.3f}\n' for c, d in durations))
    manifest_path.write_text(json.dumps(manifest, indent=1))
    total = sum(d for _, d in durations)
    print(f'{len(durations)} lines, {total:.1f} s of narration -> {OUT.relative_to(ROOT)}/', file=sys.stderr)


if __name__ == '__main__':
    main()
