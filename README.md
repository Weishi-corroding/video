# Does AI Make Us Smarter—or More Dependent?

A seven-minute animated essay built with Three.js. It has English narration, English on-screen descriptions, and optional Simplified Chinese subtitles.

AI 让我们更聪明，还是更依赖？这是一部七分钟的 Three.js 动画短文，配有英文旁白和英文画面说明，可选简体中文字幕。

Two fictional students work on the same question: *if students who use AI receive higher grades, does that prove AI improves learning?* Their behavior changes over the film. Neither student stands for a fixed personality type.

## Run it

The page uses ES modules, so serve the folder over HTTP rather than opening the file directly:

```sh
python3 -m http.server 8000
# then open http://localhost:8000
```

There is no build step and nothing to install. Three.js r186 is vendored in `vendor/three`, so the film also works offline. To publish it, turn on GitHub Pages for this repository (Settings → Pages → deploy from `main`, root).

## Chapters

| Time | Chapter | What the animation shows |
|---|---|---|
| 0:00 | One Tool, Two Approaches | Two study spaces share one AI panel. A asks for a finished answer, and B asks for a critique of a first attempt. The teacher's follow-up exposes a different gap in each student's reasoning. The scene is labelled a dramatization. |
| 0:45 | Efficiency and Independent Thinking | The workspace splits into two paths. "Time saved" tokens either flow into further reasoning or stop at a polished answer. A pros/cons list gives way to interlocking rings, and a balance of conditions (explanation required, process checked, product-only grading, deadlines) steers the flow. |
| 1:50 | Different Tasks, Different Uses | Three stations: concept, problem, argument. Each shows what AI can contribute (amber) and what the learner must still explain, verify, or perform (mint). Ends with a reflection pause. |
| 3:10 | Find the Decisive Problem | A load test reveals which weakness matters in each construction model: correlation vs. causation for A, and thin evidence plus unanswered alternative explanations for B. Minor flaws stay visible but dim. |
| 4:10 | Learning Across Time | Three linked islands: today's assignment, a later assessment, an unfamiliar problem. A changes strategy. B finds that a convincing AI suggestion cites a study that doesn't exist. Branching routes show possible trajectories, labelled as illustrative rather than measured. |
| 5:10 | Test, Reflect, Revise | A cycle of independent attempt → targeted assistance → verification → transfer → revision. Both students try a new example with the AI off and revisit it a week later. The camera then pulls back to set classroom evidence inside the broader idea of social practice. |
| 6:20 | Ask a Better Question | The workspaces reconnect. The smarter/more-dependent seesaw breaks into four questions, and the film ends on a learner making a decision and checking it. |

## Player

- **Play/pause, seeking, chapters:** click the picture or press <kbd>Space</kbd>. The seek bar is split by chapter. Use <kbd>←</kbd>/<kbd>→</kbd> for ±5 s (hold <kbd>Shift</kbd> for 15 s), <kbd>↑</kbd>/<kbd>↓</kbd> or the Chapters menu to jump between chapters, and <kbd>1</kbd>–<kbd>7</kbd> to go straight to a chapter.
- **Speed:** 0.5× to 2× (<kbd>,</kbd>/<kbd>.</kbd>).
- **Captions:** English captions (<kbd>C</kbd>) and Simplified Chinese subtitles (<kbd>Z</kbd>). When Chinese is on, the main labels also show a Chinese line.
- **Narration** (<kbd>N</kbd>) uses the browser's speech synthesis with an English system voice. You can pick the voice from the caret next to the Voice button. If no English voice exists, narration is disabled and the captions carry the full script.
- Settings persist in `localStorage`.

Each narration line is spoken as its own short utterance. If a voice is slower than the script assumes, the clock waits at the end of that line, for up to 6 s, so picture and voice stay in step. Seeking cancels speech. Resuming re-voices the current line.

URL options are useful for review: `?t=95` starts at a time, `?still` skips the start screen and stays paused, `?play` skips the start screen and plays muted, `?zh=1` turns Chinese on, and `?cc=0` turns English captions off.

## How it's built

```
index.html          player markup, import map
css/style.css       palette, typography, labels, player chrome
js/script.js        chapters + timed narration cues (English & Chinese)
js/world.js         the seven sets placed on a ring, lights, fog, camera glides
js/chapters/c1–c7   one module per chapter: builds its set, animates it from time
js/kit.js           materials, easing, props (desks, figures, AI panel, paths…)
js/labels.js        HTML labels anchored to 3D objects, screen overlays
js/timeline.js      master clock (play, pause, seek, speed)
js/narration.js     speech synthesis wrapper
js/ui.js            seek bar, menus, toggles, keys, captions, start screen
tools/check-script.mjs   pacing check for the narration script
```

Everything on screen is computed as a pure function of the master time. That's why seeking, scrubbing, and speed changes are exact and never drift. Scene animations reference narration cues by id (`c['4d'].s`), so retiming a line in `js/script.js` moves its animation too. After editing the script, run `node tools/check-script.mjs` to catch lines that are too dense to voice in their slot.

Visual language: dark teal surroundings, warm ivory models, **amber for what the AI produces**, and **mint for the learner's own reasoning and checking**. Colour follows the role, not the student, so neither student is coded as "the good one".

## Conceptual safeguards

- **Contradiction, not a pros/cons list.** Chapter 2 visibly strikes out the two-column list and replaces it with interlocking rings. Efficiency and independent thinking act on each other, and conditions (task demands, process checks, assessment format, deadlines) decide which way the influence runs.
- **Priorities from evidence.** In chapter 4 the decisive problem is the one a load test exposes, and its repair changes how the other flaws look. Minor flaws stay on screen, dimmed, because "not every weak point is a principal contradiction".
- **Teaching and assessment conditions** appear as explicit, movable weights (chapter 2), and task design is named as something that bends trajectories (chapter 5).
- **One good answer is not lasting competence.** The grade medal only appears with a question mark over later tasks. Delayed, independent, and transfer tasks are presented as stronger evidence, but still evidence, not proof (chapter 6).
- **No invented data.** The students are fictional, the trajectories carry no axes or numbers, and the film says so on screen.
- **Social practice** is distinguished from classroom checks. It is the wider ring of human activity in which ideas are tested against the world, and a classroom check is only a small, designed part of it.

## Licence

MIT (see `LICENSE`). Three.js is © three.js authors, MIT, in `vendor/three/LICENSE`.
