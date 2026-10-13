// Entry point: renderer + bloom, the world, the master timeline, narration sync,
// labels and the player UI. URL options (handy for review and testing):
//   ?t=95.5   start at a time (seconds)     ?still   skip the start screen, stay paused
//   ?zh=1     Chinese subtitles on          ?cc=0    English captions off
//   ?play     skip the start screen and play (without narration)
import * as THREE from 'three';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';
import { DURATION, CUES, cueAt } from './script.js';
import { Timeline } from './timeline.js';
import { Narrator } from './narration.js';
import { LabelLayer } from './labels.js';
import { World } from './world.js';
import { UI } from './ui.js';

function showError(msg) {
  const el = document.getElementById('error');
  el.hidden = false;
  el.textContent += (el.textContent ? '\n' : '') + msg;
}
window.addEventListener('error', (e) => showError(`Error: ${e.message}\n${e.filename || ''}:${e.lineno || ''}`));
window.addEventListener('unhandledrejection', (e) => showError(`Unhandled: ${e.reason?.stack || e.reason}`));

const params = new URLSearchParams(location.search);
const canvas = document.getElementById('gl');

let renderer;
try {
  renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: 'high-performance' });
} catch (err) {
  showError('WebGL is not available in this browser, so the animation cannot run.');
  throw err;
}
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFShadowMap;
renderer.toneMapping = THREE.NeutralToneMapping;
renderer.toneMappingExposure = 1.45; // applied after bloom, so lit surfaces stay below the bloom threshold

const labels = new LabelLayer(document.getElementById('anchors'), document.getElementById('overlays'), document.getElementById('captions'));
const dip = document.getElementById('dip');
const world = new World(labels);
const camera = new THREE.PerspectiveCamera(38, 16 / 9, 0.1, 400);

const composer = new EffectComposer(renderer);
composer.addPass(new RenderPass(world.scene, camera));
const bloom = new UnrealBloomPass(new THREE.Vector2(1280, 720), 0.55, 0.5, 0.86);
composer.addPass(bloom);
composer.addPass(new OutputPass());

const timeline = new Timeline(DURATION);
const narrator = new Narrator();
const ui = new UI({ timeline, narrator, params });

// ---------- narration ↔ timeline sync ----------
// A cue is voiced when playback reaches it. If the voice is still talking when
// the cue's slot ends, the clock waits (up to MAX_HOLD) so speech and picture
// stay together, whatever the voice's natural pace.
const MAX_HOLD = 6;
let spokenCue = -1;
let held = 0;

function voiceCue(i) {
  spokenCue = i;
  narrator.speak(i, timeline.speed);
}
// Seeking while playing re-voices the line it lands in, as resuming does, so
// keyboard seeks and seek-bar drags behave the same. The short settle keeps a
// burst of arrow presses from stuttering.
let seekVoice = 0;
timeline.addEventListener('seek', () => {
  narrator.cancel();
  held = 0;
  clearTimeout(seekVoice);
  const i = cueAt(timeline.time);
  spokenCue = i; // the frame loop leaves this cue to the settle timer below
  if (i < 0 || !timeline.playing || !narrator.enabled) return;
  seekVoice = setTimeout(() => {
    if (timeline.playing && narrator.enabled && cueAt(timeline.time) === i) voiceCue(i);
  }, 250);
});
timeline.addEventListener('pause', () => narrator.cancel());
timeline.addEventListener('play', () => {
  const i = cueAt(timeline.time);
  if (i >= 0 && narrator.enabled) voiceCue(i); // resume re-voices the current line
});
ui.dispatchVoiceToggle = (on) => {
  if (!on || !timeline.playing) return;
  const i = cueAt(timeline.time);
  if (i >= 0 && timeline.time - CUES[i].start < 1.5) voiceCue(i);
};

function holdForVoice(prev, next, dt) {
  if (!narrator.enabled || narrator.cue < 0 || !narrator.speaking) { held = 0; return next; }
  const end = CUES[narrator.cue].end;
  if (next < end) { held = 0; return next; }
  held += dt;
  if (held > MAX_HOLD) { narrator.cancel(); held = 0; return next; }
  return Math.max(prev, end);
}

// ---------- sizing ----------
let W = 0, H = 0;
function resize() {
  W = window.innerWidth;
  H = window.innerHeight;
  renderer.setSize(W, H, false);
  composer.setSize(W, H);
  bloom.setSize(W, H);
  camera.aspect = W / H;
  camera.updateProjectionMatrix();
}
window.addEventListener('resize', resize);
resize();

// ---------- start options ----------
if (params.has('t')) timeline.seek(parseFloat(params.get('t')) || 0);
if (params.has('still')) ui.skipStart();
if (params.has('play')) { ui.skipStart(); timeline.play(); } // muted autoplay, for review/testing

// ---------- loop ----------
let last = performance.now();
function frame(now) {
  const dt = Math.min(0.1, Math.max(0, (now - last) / 1000));
  last = now;

  timeline.tick(dt, holdForVoice);
  const t = timeline.time;

  if (timeline.playing && narrator.enabled) {
    const i = cueAt(t);
    if (i >= 0 && i !== spokenCue) {
      if (t - CUES[i].start < 1.0) voiceCue(i);
      else spokenCue = i;
    }
  }

  world.update(t);
  const { p, l } = world.cameraPose(t, W / H);
  camera.position.copy(p);
  camera.lookAt(l);
  world.applyEnvironment(l);
  composer.render();
  dip.style.opacity = world.dip.toFixed(3);
  labels.update(camera, W, H, world.activeLabels);
  ui.update(t);
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);

// expose for debugging in the console
window.essay = { timeline, narrator, world, ui };
