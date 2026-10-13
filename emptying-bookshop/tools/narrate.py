#!/usr/bin/env python3
"""Record the narration with an offline neural text-to-speech engine.

usage: ./build/emptying-bookshop --lang zh --cues | ~/.venvs/kokoro/bin/python tools/narrate.py --lang zh
       ./build/emptying-bookshop --lang en --cues | ~/.venvs/piper/bin/python tools/narrate.py --lang en

Reads "id<TAB>line" from stdin and writes narration/<lang>/<id>.wav (trimmed, 16-bit mono) and
narration/<lang>/durations.tsv, which the film uses to give every line enough room. Lines whose
text and voice have not changed since the last run are kept.

Chinese uses Kokoro v1.1-zh (Apache-2.0; its Chinese training data was given for this use by
LongMaoData). English uses Piper with the en_GB-cori-high voice (trained on public-domain LibriVox
recordings). Setup (once):

  python3 -m venv ~/.venvs/kokoro
  ~/.venvs/kokoro/bin/pip install torch --index-url https://download.pytorch.org/whl/cpu
  ~/.venvs/kokoro/bin/pip install "kokoro>=0.9.4" "misaki[zh]>=0.9.4" soundfile

  python3 -m venv ~/.venvs/piper && ~/.venvs/piper/bin/pip install piper-tts
  mkdir -p ~/.cache/piper-voices && cd ~/.cache/piper-voices && ~/.venvs/piper/bin/python -m piper.download_voices en_GB-cori-high
"""
import argparse
import array
import hashlib
import io
import json
import pathlib
import sys
import wave

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Respellings so the voice says names the way they are said, without changing the captions.
SAY = {
    'en': {'Zedong': 'Zuh-dong'},
    'zh': {},
}
# Words the Chinese frontend would read with a neutral tone that standard Mandarin gives a full tone.
FULL_TONE = ['分析']


def trim(samples, rate, threshold=0.01, pad=0.06):
    """Cut leading and trailing silence, keeping a short pad."""
    limit = int(threshold * 32767)
    loud = [i for i, s in enumerate(samples) if abs(s) > limit]
    if not loud:
        return samples
    p = int(pad * rate)
    return samples[max(0, loud[0] - p):min(len(samples), loud[-1] + p)]


class Kokoro:
    REPO = 'hexgrad/Kokoro-82M-v1.1-zh'
    RATE = 24000

    def __init__(self, voice, speed):
        from kokoro import KModel, KPipeline
        self.voice, self.speed = voice, speed
        model = KModel(repo_id=self.REPO).eval()
        en = KPipeline(lang_code='a', repo_id=self.REPO, model=False)
        self.pipe = KPipeline(lang_code='z', repo_id=self.REPO, model=model,
                              en_callable=lambda text: next(en(text)).phonemes)
        sandhi = self.pipe.g2p.frontend.tone_modifier
        for w in FULL_TONE:
            sandhi.must_neural_tone_words.discard(w)

    def pace(self, n):
        # The model rushes long inputs (it saw few beyond ~100 phonemes in training); slow those down.
        k = 1.0 if n <= 83 else max(0.8, 1 - (n - 83) / 500)
        return k * self.speed

    def __call__(self, text):
        import numpy as np
        parts = [r.audio.numpy() for r in self.pipe(text, voice=self.voice, speed=self.pace, split_pattern=None)]
        audio = np.concatenate(parts)
        pcm = (np.clip(audio, -1, 1) * 32767).astype('<i2')
        return array.array('h', pcm.tobytes()), self.RATE


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
    ap.add_argument('--voice', help='Kokoro voice (default zm_010) or Piper model file (default en_GB-cori-high)')
    ap.add_argument('--speed', type=float, default=1.0, help='Kokoro: above 1 speaks faster')
    ap.add_argument('--length-scale', type=float, default=1.06, help='Piper: above 1 speaks more slowly')
    args = ap.parse_args()

    out = ROOT / 'narration' / args.lang
    out.mkdir(parents=True, exist_ok=True)
    lines = [l.rstrip('\n').split('\t', 1) for l in sys.stdin if l.strip()]
    manifest_path = out / 'manifest.json'
    manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}

    # Models load only when a line needs recording; the cache key describes the settings instead.
    if args.lang == 'zh':
        voice = args.voice or 'zm_010'
        setting = f'kokoro-v1.1-zh|{voice}|{args.speed}|full:{",".join(FULL_TONE)}'
        make = lambda: Kokoro(voice, args.speed)
    else:
        voice = args.voice or str(pathlib.Path.home() / '.cache/piper-voices/en_GB-cori-high.onnx')
        setting = f'{voice}|{args.length_scale}'
        make = lambda: Piper(voice, args.length_scale)
    tts = None

    durations = []
    for cue, text in lines:
        spoken = text
        for word, say in SAY[args.lang].items():
            spoken = spoken.replace(word, say)
        key = hashlib.sha1(f'{setting}|{spoken}'.encode()).hexdigest()
        wav_path = out / f'{cue}.wav'
        if manifest.get(cue) != key or not wav_path.exists():
            tts = tts or make()
            samples, rate = tts(spoken)
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
    (out / 'durations.tsv').write_text(''.join(f'{c}\t{d:.3f}\n' for c, d in durations))
    manifest_path.write_text(json.dumps(manifest, indent=1))
    total = sum(d for _, d in durations)
    print(f'{len(durations)} lines, {total:.1f} s of narration -> {out.relative_to(ROOT)}/', file=sys.stderr)


if __name__ == '__main__':
    main()
