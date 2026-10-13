# The Emptying Bookshop

日渐冷清的书店：一部七分钟的动画短片，讲述如何运用马克思主义基本原理提升辩证思维能力。整部片子是一个
立在精装书上的纸模型校园：开场时纸模型从书封上立起来，章节之间像翻书一样翻页。由本仓库的 C++/Vulkan
引擎渲染为 MP4，配有中文字幕和合成配乐；中文旁白的录制方法见下文“Narration”。也可以生成英文版。

A seven-minute animated film on how to apply Marxist principles to improve dialectical thinking
skills. The campus is a paper model built on top of a closed hardback book: it stands up from the
cover at the start, and the chapters turn like pages. It is rendered to MP4 by the C++/Vulkan engine
in [`../engine`](../engine), with Chinese captions and a synthesized score; see "Narration" for adding
a voice. An English version can be made too.

Jia runs a student second-hand bookshop, and fewer people come in each week. The film follows
Jia's choices. First come three quick reactions, then an investigation, the contradiction inside
the problem, small tests that add up, and a look back at the shop's history. Each principle is
shown at work before it is named.

## Make the film

Requirements: CMake 3.20+, a C++20 compiler, Vulkan headers and a driver, glslc, FreeType,
HarfBuzz, libpng and ffmpeg. On Debian or Ubuntu:

```sh
sudo apt install cmake ninja-build g++ libvulkan-dev glslc libfreetype-dev libharfbuzz-dev libpng-dev ffmpeg mesa-vulkan-drivers
```

Then, from this folder:

```sh
cmake -S . -B build -G Ninja && ninja -C build
./build/emptying-bookshop              # → out/emptying-bookshop.mp4 (Chinese)
./build/emptying-bookshop --lang en    # → out/emptying-bookshop.en.mp4 (English)
```

On a GPU the film renders in a few minutes. With Mesa's software rasteriser (no GPU) it runs at a
few frames a second at 1080p, so a version takes over an hour.

Useful options (`--help` lists them all):

- `--still 95,240` writes PNG stills at those times to `out/stills/`.
- `--from 180 --to 200` renders part of the film.
- `--size 1280x720`, `--fps 30` and `--crf 18` set the size, frame rate and quality.
- `--audio-only out/soundtrack.wav` writes the soundtrack (narration and score) without rendering any
  pictures; add `--stems` to also get the voice and the music on their own.
- `--no-music` leaves the score out; `--no-audio` makes a silent video.
- `--timeline` prints when each line is spoken.
- `--check` prints the timing table and checks the script. It flags lines with too little room to
  be spoken and Chinese lines too long for two subtitle lines.

### Narration

The Chinese version has no narration yet. It plays with captions and the score, and the score is
mixed louder when there is no voice. Its timing follows a comfortable reading pace (4.4 characters a
second), and it changes to fit the recordings once they are imported. A first try with an offline
speech model (Kokoro) sounded too mechanical, so it is not used.

Recordings come in through `tools/narrate.py`: one file per line, named by the line's id (`1a.wav`,
`1b.wav`, …; wav, flac, mp3, m4a or ogg):

```sh
./build/emptying-bookshop --timeline            # the lines, their ids and where they fall
./build/emptying-bookshop --lang zh --cues | python3 tools/narrate.py --lang zh --import ~/recordings/zh
./build/emptying-bookshop --check               # every line fits?
./build/emptying-bookshop --audio-only out/soundtrack.wav --stems   # listen before rendering
```

The tool converts each file to 48 kHz mono, trims the silence at both ends and writes
`narration/zh/durations.tsv` (commit this file; the recordings stay out of the repository). Every
line is brought to the same speech level in the mix, and the score dips under it.

Ways to make the recordings, best first:

1. **A human narrator.** This sounds best by far, and the script is short: 57 lines, about 1,550
   characters, about six minutes of speech. Record in a quiet, soft room, one take per line, with
   about half a second of silence before and after. Speak at a calm, even pace (about four
   characters a second) and in a conversational tone rather than a broadcast one. Export at 48 kHz.
   A phone in a wardrobe full of clothes does better than a good microphone in an echoing room.
2. **An expressive open speech model, run locally.** These are much more natural than older
   text-to-speech. Many can clone a voice from a short sample, but only clone a voice with its
   owner's permission. Check each licence against how the film will be used:
   - [CosyVoice 2 / Fun-CosyVoice 3](https://github.com/FunAudioLLM/CosyVoice) (Alibaba Tongyi Lab):
     weights under Apache-2.0, strong Mandarin, and tone and pace can be set by instruction.
   - [IndexTTS-2](https://github.com/index-tts/index-tts) (bilibili): emotion control, and the
     length of each line can be set, which suits narration timed to a picture. It uses the bilibili
     Model Use License: royalty-free, with extra terms for very large companies.
   - [GPT-SoVITS](https://github.com/RVC-Boss/GPT-SoVITS) (MIT): fine-tuned on a minute or more of
     one consenting speaker, it reads in that speaker's voice.
   - Non-commercial only: [F5-TTS](https://github.com/SWivid/F5-TTS) and MaskGCT weights (CC BY-NC
     4.0), Spark-TTS and Fish Audio's OpenAudio S1-mini (CC BY-NC-SA 4.0). MegaTTS3 is Apache-2.0
     but its voice-cloning encoder is not published.
3. **A cloud speech service with narration styles.** Azure AI Speech (for example
   `zh-CN-YunxiNeural` or `zh-CN-XiaoxiaoNeural`, with SSML for pauses, emphasis and speaking
   styles), Volcano Engine's Doubao speech synthesis, Alibaba Cloud's CosyVoice service, Tencent
   Cloud and iFlytek all have natural Mandarin voices. They are paid services, and their terms say
   how the audio may be used.

With any speech model, these help it sound less mechanical:
- Generate a chapter as one piece and cut it into lines, so the prosody carries across sentences.
- Write numbers and names the way they are said.
- Use the model's slower or "narration" setting.
- Listen to every line; re-generate any that stumble, and fix misread polyphonic characters (多音字)
  with a homophone in the spoken text only.

English uses [Piper](https://github.com/OHF-Voice/piper1-gpl) with the `en_GB-cori-high` voice
(trained on public-domain LibriVox recordings); a human narrator would be better here too:

```sh
python3 -m venv ~/.venvs/piper && ~/.venvs/piper/bin/pip install piper-tts
mkdir -p ~/.cache/piper-voices && (cd ~/.cache/piper-voices && ~/.venvs/piper/bin/python -m piper.download_voices en_GB-cori-high)
./build/emptying-bookshop --lang en --cues | ~/.venvs/piper/bin/python tools/narrate.py --lang en
```

### Music

The score is synthesized by the program each time (`src/music.cpp`, with the engine's
`eng/synth`): felt piano, plucked strings, a string pad, a music box and a bass, through one reverb.
It is written on the script's timing, so it follows the narration when a line changes, and it dips
under the voice. Paper sounds join it: a flap as each piece of the model stands up, and a rustle with
every page turn. One theme in D major runs through the film:

1. The theme over the campus, turning minor as the visitors thin out.
2. A quick plucked figure for the quick reactions. It freezes with the snapshot: a shutter, then one
   held chord while the crowd stands still.
3. A walking groove for the investigation, a chime as each finding appears, and a clock that ticks up
   to five.
4. No beat. The two sides of the contradiction are two chords a tritone apart, one in each ear; they
   resolve to a warm chord when the cables turn gold.
5. A music-box note for every bead that drops into the jar, so the regulars can be heard adding up.
   When the beads link up, the theme returns in full.
6. The theme at stage 1, in the dominant at stage 2, and an octave higher at stage 3: a return
   that has climbed.
7. A note for each pin, the theme once more, and a long final chord.

### The look

The film opens on the closed book with the campus folded flat on its cover, printed-looking. The
pieces spring up in a wave from the middle outward, then the crowd walks on. Between chapters the
picture turns like a page. It curls up from the bottom-right corner, its cream back showing the
print faintly through, and it casts a shadow on the next chapter, already going on beneath it. Each
chapter opens with a cut hidden under the page.

Everything is paper. The engine gives paper materials a fibre texture and a fine relief, and lets
light wrap a little past the edge of shadow, as it does through thin paper. The campus is built on a
closed hardback with cloth boards, a striped page block, a gilt-banded spine and a ribbon bookmark.
- **Buildings** are folded card, with gable roofs, framed windows, sills and doors.
- **Trees** are folded cones, faceted paper balls, or two crossed cut-outs.
- **People** are paper dolls with swinging arms.
- **Clouds** are cut from card.

The camera works like a macro lens on a model: ambient occlusion darkens creases and contact points,
and a shallow depth of field focuses on what the camera looks at (stronger in wide views, which gives
the tilt-shift look of a miniature). The ideas are paper too: the threads are yarn, the two sides of
the contradiction are rolled columns, and the shop's history is a gold paper ribbon. Labels are
cream tags taped on and pinned to what they name, and quotations are printed cards set in a Chinese
serif.

## Chapters

Times are for the Chinese version at the reading pace; they shift a little once narration is imported.

| Time | Chapter | What the animation shows |
|---|---|---|
| 0:00 | An Emptying Shop | The paper campus standing up from the book's cover, Jia, and a floating vignette of the shop's origin as a free swap shelf in a dorm lounge. A time-lapse shows fewer visitors week by week. |
| 0:46 | Three Quick Reactions | A heavy verdict, "Nobody reads anymore", drops in front of the shop while the campus dims around it (*isolated*). Another campus's half-price banner is copied onto the shop and changes nothing (*no concrete analysis*; Lenin's "living soul of Marxism"). After one quiet week, the frame freezes into a snapshot (*static*). These are named as metaphysical thinking in the Marxist sense. |
| 2:03 | Go and Find Out | "No investigation, no right to speak." Jia asks students and makes four findings: the shop moved off the main path; it closes at 5, as classes end; readings went online; and there is nowhere to study together in the evening. Each finding becomes a thread back to the shop, and the threads have different thicknesses (*universal connection, unequal weight*). |
| 3:09 | The Pull Inside the Problem | Two pillars pull the shop two ways: what the shop is (a daytime sales counter) and what students now need. An arch shows that each side depends on the other (*unity of opposites*). Small problems (sign, website, prices) are re-hung on the main cables, which turn gold (*principal contradiction, under these conditions*). An outer ring of arrows travels inward (*external causes work through internal causes*). |
| 4:22 | Test, Accumulate, Revise | Three tests: open until 9, a swap shelf, and patio tables. Beads drop into a jar as regulars accumulate, then link into a network (*quantity → quality*). Noise complaints reach the library, and Jia revises the layout (*practice, knowledge, again practice*). |
| 5:41 | Back, but Higher | The shop's history as a helix: swap shelf → shop → swap-and-study room. From above it looks like a circle, and from the side it climbs (*negation of the negation*). |
| 6:34 | Your Own Bookshop | Six numbered pins and a summary of Jia's turning points, a question for the viewer, and one action for this week. |

## How it's built

```
CMakeLists.txt           builds ../engine and the film
src/main.cpp             command line, the frame loop, the soundtrack mix, encoding
src/script.cpp           chapters and timed narration cues (Chinese and English)
src/music.cpp            the score
src/pageturn.cpp         the page turn between chapters
src/world.cpp            light by time of day, the book and the campus, shop, crowd, Jia, camera rig
src/chapters/c1–c7.cpp   one file per chapter: builds its props, labels and keys, and animates them from time
src/kit.cpp              easing, paper materials, props (paper dolls, trees, shelves, signs, cut card, threads…)
src/overlay.cpp          labels anchored to 3D points or the screen, panels, title cards, captions
tools/narrate.py         imports recorded narration (or makes English with Piper)
tools/subset-cjk.py      cuts Noto Sans and Noto Serif CJK SC down to the characters the film uses
fonts/                   Inter, Source Serif 4, and Noto Sans / Serif SC subsets
narration/<lang>/        the length of each recorded line (and, locally, the recordings)
```

Every frame is a pure function of film time, so any part of the film can be rendered on its own.
Chapters register camera, light, crowd, shop-state and Jia keyframes with the world, and every
key time comes from a narration cue (`c("3d").s`). Cue times are laid out from the length of each
line in the film's language: at least 4.4 Chinese characters (or 2.6 English words) a second, and
longer if the recorded voice needs it. Editing a line in `src/script.cpp` therefore retimes it and
everything after it, and the music follows. After editing the script, re-run `tools/narrate.py` (it
re-records only changed lines), `tools/subset-cjk.py` if you added Chinese characters, and `--check`.

Colour carries meaning, and each coloured label also says in words what it is (the tape and pin on
each tag are in its colour):
- **teal:** a fact Jia found
- **coral:** an assumption or a problem
- **gold:** a named principle
- **blue / violet:** the two sides of the shop's contradiction

Jia wears a green apron and carries a notebook.

## Conceptual notes

- **"Metaphysical"** is used in the Marxist sense, as the opposite of dialectics (seeing things as isolated, static and one-sided), and the screen says so.
- **Principal contradiction.** The principal contradiction is not presented as "the biggest problem". The film names its two sides, shows that they depend on each other and pull apart, and shows why it shapes the smaller problems: a brighter sign brings people to a shop that still doesn't fit their day. It is named "under these conditions".
- **Internal and external causes.** "Nobody reads anymore" returns as a real outside condition (readings moving online) that acts through how the shop meets its own contradiction.
- **Quantity and quality.** The change in kind is shown as beads becoming a network: a pile of regulars turns into a place where people meet. The only numbers are the story's own ("two regulars, then five").
- **Negation of the negation** is presented as a way to read development, not a formula that predicts it.
- **Quotations:**
  - Mao Zedong, *Oppose Book Worship* (1930): "No investigation, no right to speak."
  - V. I. Lenin, *Kommunismus* (1920): a concrete analysis of a concrete situation as "the living soul of Marxism".
  - Mao Zedong, *On Practice* (1937): "practice, knowledge, again practice, and again knowledge".
- Jia, the shop and the campus are fictional, and the film says so on screen.

## Licence

MIT (see `LICENSE`). Inter, Source Serif 4, Noto Sans CJK SC and Noto Serif CJK SC are under the
SIL Open Font License 1.1 (`fonts/OFL-*.txt`).
