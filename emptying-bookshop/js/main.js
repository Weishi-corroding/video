import { chapters, cues, c, DURATION, cueAt } from './script.js';
import { createWorld, P } from './world.js';
import { Labels } from './labels.js';
import { Timeline } from './timeline.js';
import { Narrator } from './narration.js';
import { createUI } from './ui.js';
import { win } from './kit.js';
import * as c1 from './chapters/c1.js';
import * as c2 from './chapters/c2.js';
import * as c3 from './chapters/c3.js';
import * as c4 from './chapters/c4.js';
import * as c5 from './chapters/c5.js';
import * as c6 from './chapters/c6.js';
import * as c7 from './chapters/c7.js';

const $ = (id) => document.getElementById(id);
const params = new URLSearchParams(location.search);
const KEY = 'emptying-bookshop-settings';

let saved = {};
try { saved = JSON.parse(localStorage.getItem(KEY) || '{}'); } catch { saved = {}; }
const state = {
  cc: saved.cc ?? true,
  zh: saved.zh ?? false,
  narr: false,
  voiceName: saved.voiceName ?? null,
  rate: [0.5, 0.75, 1, 1.25, 1.5, 2].includes(saved.rate) ? saved.rate : 1,
};
if (params.has('zh')) state.zh = params.get('zh') !== '0';
if (params.has('cc')) state.cc = params.get('cc') !== '0';
if (params.has('clean')) $('player').classList.add('clean');

// Canvas signs are drawn with Inter, so wait for it (but never for long).
await Promise.race([
  Promise.all([
    document.fonts.load('700 40px Inter'),
    document.fonts.load('600 40px "Source Serif 4"'),
    document.fonts.load('italic 500 40px "Source Serif 4"'),
  ]),
  new Promise((r) => setTimeout(r, 2500)),
]);

const W = createWorld($('stage'));
const L = new Labels($('labels'), W.camera);
const ctx = { W, L, c, chapters, P };
const mods = [c1, c2, c3, c4, c5, c6, c7].map((m) => m.build(ctx));

// Chapter tag, top left, for the first seconds of every chapter (chapter 1 waits for the title card).
for (const ch of chapters) {
  const start = ch.n === 1 ? c['1a'].s + 0.4 : ch.s + 0.3;
  L.add({
    kind: 'chapter', screen: [0.03, 0], anchor: [0, 0], stem: false,
    html: `<div class="num">Chapter ${ch.n}</div><div class="ttl">${ch.title}</div><div class="zh">${ch.zh}</div>`,
    vis: (t) => win(t, start, start + 4.6, 0.6, 0.9),
  });
}
W.finalize();

const timeline = new Timeline(DURATION);
timeline.rate = state.rate;
const narrator = new Narrator();
const applyVoice = () => { if (narrator.available) narrator.voice = narrator.pickDefault(state.voiceName); };
applyVoice();

const save = () => {
  try { localStorage.setItem(KEY, JSON.stringify({ cc: state.cc, zh: state.zh, voiceName: state.voiceName, rate: state.rate })); } catch { /* private mode */ }
};

let dirty = true;
let lastT = 0;
let waited = 0;

// Voice the cue under the playhead, but only if we are early enough in it for the voice to finish in step.
function voiceCurrent() {
  if (!state.narr || !timeline.playing) return;
  const q = cueAt(timeline.time);
  if (q && timeline.time - q.s < 0.5 * (q.e - q.s)) narrator.speak(q, timeline.rate);
}

function onChange(what) {
  save();
  if (what === 'zh') L.setZh(state.zh);
  if (what === 'narr') { narrator.cancel(); voiceCurrent(); }
  if (what === 'voice') { applyVoice(); narrator.cancel(); voiceCurrent(); }
  dirty = true;
}

const ui = createUI({ timeline, narrator, chapters, duration: DURATION, state, onChange });
const prevVoices = narrator.onvoices;
narrator.onvoices = () => { prevVoices?.(); applyVoice(); };
L.setZh(state.zh);

timeline.on((type) => {
  if (type === 'seek') { narrator.cancel(); waited = 0; lastT = timeline.time; voiceCurrent(); }
  if (type === 'play') { lastT = timeline.time; voiceCurrent(); }
  if (type === 'pause' || type === 'end') narrator.cancel();
  dirty = true;
});

// ----- sizing -----
function resize() {
  const w = window.innerWidth, h = window.innerHeight;
  W.resize(w, h);
  L.resize(w, h);
  dirty = true;
}
window.addEventListener('resize', resize);
resize();

// Labels must stay clear of the caption band, sized for the lines that can appear there.
function safeArea() {
  const w = window.innerWidth, h = window.innerHeight;
  const fs = Math.min(24, Math.max(15, w * 0.0155));
  const capBottom = w <= 640 ? 70 : 74;
  let band = 0;
  if (state.cc) band += 2 * 1.32 * fs;
  if (state.zh) band += 2 * 1.26 * fs;
  band += band ? 16 : 0;
  return { top: 14, left: 14, right: 14, bottom: h - capBottom - band - 10 };
}

// ----- frame loop -----
let prev = performance.now();
let rendered = -1;
function frame(now) {
  const dt = Math.min(0.25, (now - prev) / 1000);
  prev = now;
  if (timeline.playing) {
    // While a line is still being spoken, do not start the next one; wait up to 6 s for the voice.
    let limit = Infinity;
    if (state.narr && narrator.speaking) {
      const q = c[narrator.speaking];
      const next = cues[cues.indexOf(q) + 1];
      const boundary = next ? next.s - 0.02 : DURATION;
      if (timeline.time >= boundary - 0.001) {
        waited += dt;
        if (waited > 6) { narrator.cancel(); waited = 0; } else limit = boundary;
      } else { limit = boundary; waited = 0; }
    } else waited = 0;
    timeline.advance(dt, limit);
    if (state.narr) {
      let crossed = null;
      for (const q of cues) if (q.s > lastT && q.s <= timeline.time) crossed = q;
      if (crossed) narrator.speak(crossed, timeline.rate);
    }
  }
  lastT = timeline.time;
  const t = timeline.time;
  if (dirty || t !== rendered || timeline.playing) {
    W.update(t);
    for (const m of mods) m.update(t);
    W.applyFx();
    L.update(t, safeArea());
    const speaking = state.narr && narrator.speaking ? c[narrator.speaking] : null;
    ui.update(t, speaking || cueAt(t));
    W.render();
    rendered = t;
    dirty = false;
  }
  requestAnimationFrame(frame);
}

// ----- start -----
const t0 = parseFloat(params.get('t'));
if (Number.isFinite(t0)) timeline.seek(t0);
$('loading').classList.add('hidden');
if (params.has('still')) {
  $('start').classList.add('hidden');
} else if (params.has('play')) {
  $('start').classList.add('hidden');
  timeline.play();
} else {
  ui.showStart(() => timeline.play());
}
requestAnimationFrame(frame);
window.__film = { timeline, W, L, state, c, chapters };
