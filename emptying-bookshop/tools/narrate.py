#!/usr/bin/env python3
"""Bring in the narration: import recordings (a human narrator, or any speech tool), or make English
ones with Piper.

usage:
  ./build/emptying-bookshop --lang zh --cues | python3 tools/narrate.py --lang zh --import ~/recordings/zh
  ./build/emptying-bookshop --lang en --cues | ~/.venvs/piper/bin/python tools/narrate.py --lang en

Reads "id<TAB>line" from stdin and writes narration/<lang>/<id>.wav (trimmed, 16-bit mono, 48 kHz)
and narration/<lang>/durations.tsv, which the film uses to give every line enough room.

--import DIR takes DIR/<id>.wav (or .flac, .mp3, .m4a, .ogg) for each line and converts it with
ffmpeg; lines without a file are reported and left out. See README.md, "Narration", for ways to
make the recordings.

Without --import, English lines are synthesized with Piper and the en_GB-cori-high voice (trained
on public-domain LibriVox recordings). Lines whose text and voice have not changed since the last run
are kept. Setup (once):
  python3 -m venv ~/.venvs/piper && ~/.venvs/piper/bin/pip install piper-tts
  mkdir -p ~/.cache/piper-voices && cd ~/.cache/piper-voices && ~/.venvs/piper/bin/python -m piper.download_voices en_GB-cori-high
"""
import argparse
import array
import hashlib
import io
import json
import pathlib
import subprocess
import sys
import wave

ROOT = pathlib.Path(__file__).resolve().parent.parent
RATE = 48000

# Respellings so the English voice says names the way they are said, without changing the captions.
SAY = {'Zedong': 'Zuh-dong'}


def respell(text):
    for word, say in SAY.items():
        text = text.replace(word, say)
    return text


def trim(samples, rate, threshold=0.01, pad=0.08):
    """Cut leading and trailing silence, keeping a short pad."""
    limit = int(threshold * 32767)
    loud = [i for i, s in enumerate(samples) if abs(s) > limit]
    if not loud:
        return samples
    p = int(pad * rate)
    return samples[max(0, loud[0] - p):min(len(samples), loud[-1] + p)]


def decode(path):
    """Any audio file to 16-bit mono samples at RATE, with ffmpeg."""
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', str(path), '-ac', '1', '-ar', str(RATE), '-f', 's16le', '-'],
                         check=True, capture_output=True).stdout
    return array.array('h', raw), RATE


class Piper:
    def __init__(self, voice, length_scale):
        from piper import PiperVoice, SynthesisConfig
        self.voice = PiperVoice.load(voice)
        self.config = SynthesisConfig(length_scale=length_scale)

    def __call__(self, text):
        buf = io.BytesIO()
        with wave.open(buf, 'wb') as w:
            self.voice.synthesize_wav(text, w, syn_config=self.config)
        buf.seek(0)
        with wave.open(buf, 'rb') as r:
            return array.array('h', r.readframes(r.getnframes())), r.getframerate()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lang', choices=['zh', 'en'], default='zh')
    ap.add_argument('--import', dest='source', metavar='DIR', help='take the recordings from DIR/<id>.wav (or .flac, .mp3, .m4a, .ogg)')
    ap.add_argument('--voice', default=str(pathlib.Path.home() / '.cache/piper-voices/en_GB-cori-high.onnx'), help='Piper model file')
    ap.add_argument('--length-scale', type=float, default=1.06, help='Piper: above 1 speaks more slowly')
    args = ap.parse_args()
    if not args.source and args.lang != 'en':
        sys.exit('Chinese narration is imported: pass --import DIR with one recording per line (see README.md).')

    out = ROOT / 'narration' / args.lang
    out.mkdir(parents=True, exist_ok=True)
    lines = [l.rstrip('\n').split('\t', 1) for l in sys.stdin if l.strip()]
    manifest_path = out / 'manifest.json'
    manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
    tts = None
    durations, missing = [], []
    for cue, text in lines:
        wav_path = out / f'{cue}.wav'
        if args.source:
            src = next((p for ext in ('wav', 'flac', 'mp3', 'm4a', 'ogg') if (p := pathlib.Path(args.source) / f'{cue}.{ext}').exists()), None)
            if src is None:
                missing.append(cue)
                continue
            key = hashlib.sha1(f'import|{src.stat().st_size}|{src.stat().st_mtime_ns}|{text}'.encode()).hexdigest()
        else:
            key = hashlib.sha1(f'{args.voice}|{args.length_scale}|{respell(text)}'.encode()).hexdigest()
        if manifest.get(cue) != key or not wav_path.exists():
            if args.source:
                samples, rate = decode(src)
            else:
                tts = tts or Piper(args.voice, args.length_scale)
                samples, rate = tts(respell(text))
            samples = trim(samples, rate)
            with wave.open(str(wav_path), 'wb') as w:
                w.setnchannels(1)
                w.setsampwidth(2)
                w.setframerate(rate)
                w.writeframes(samples.tobytes())
            manifest[cue] = key
            print(f'{cue}: {"imported" if args.source else "recorded"}', file=sys.stderr)
        with wave.open(str(wav_path), 'rb') as r:
            durations.append((cue, r.getnframes() / r.getframerate()))
    (out / 'durations.tsv').write_text(''.join(f'{c}\t{d:.3f}\n' for c, d in durations))
    manifest_path.write_text(json.dumps(manifest, indent=1))
    total = sum(d for _, d in durations)
    print(f'{len(durations)} lines, {total:.1f} s of narration -> {out.relative_to(ROOT)}/', file=sys.stderr)
    if missing:
        print(f'No recording for {len(missing)} line(s): {" ".join(missing)}', file=sys.stderr)


if __name__ == '__main__':
    main()
