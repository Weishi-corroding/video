// Pacing and consistency check for the narration script.
// Usage: node tools/check-script.mjs
import { readFileSync, readdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { chapters, cues, c, DURATION, words, WPS } from '../js/script.js';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const SPOKEN_WPS = 2.75;   // a typical English system voice at rate 1
const ZH_MAX = 48;         // longest Chinese subtitle that still fits on two lines
const problems = [];

const fmt = (s) => `${Math.floor(s / 60)}:${String(Math.floor(s % 60)).padStart(2, '0')}`;

const ids = new Set();
cues.forEach((q, i) => {
  if (ids.has(q.id)) problems.push(`${q.id}: duplicate id`);
  ids.add(q.id);
  if (!q.zh || !q.zh.trim()) problems.push(`${q.id}: missing Chinese line`);
  if ([...q.zh].length > ZH_MAX) problems.push(`${q.id}: Chinese line has ${[...q.zh].length} characters (max ${ZH_MAX})`);
  const next = cues[i + 1];
  const room = (next ? next.s : DURATION) - q.s;
  const need = words(q.en) / SPOKEN_WPS;
  if (need > room) problems.push(`${q.id}: about ${need.toFixed(1)} s to speak but only ${room.toFixed(1)} s before the next line`);
  if (next && next.s < q.e) problems.push(`${q.id}: overlaps ${next.id}`);
});

// Every cue id referenced by a chapter module must exist.
const chapterDir = join(root, 'js', 'chapters');
for (const f of readdirSync(chapterDir)) {
  const src = readFileSync(join(chapterDir, f), 'utf8');
  for (const m of src.matchAll(/c\[['"](\w+)['"]\]/g)) {
    if (!c[m[1]]) problems.push(`${f}: refers to unknown cue '${m[1]}'`);
  }
}

const total = cues.reduce((n, q) => n + words(q.en), 0);
console.log(`${chapters.length} chapters, ${cues.length} cues, ${total} words, ${fmt(DURATION)} (${DURATION.toFixed(1)} s)`);
console.log(`Average ${(total / (DURATION / 60)).toFixed(0)} words per minute over the film; layout pace ${WPS} words/s.`);
for (const ch of chapters) {
  const w = ch.cues.reduce((n, q) => n + words(q.en), 0);
  console.log(`  ${ch.n}. ${fmt(ch.s)}–${fmt(ch.e)}  ${ch.title.padEnd(30)} ${String(ch.cues.length).padStart(2)} cues ${String(w).padStart(4)} words`);
}
if (problems.length) {
  console.log(`\n${problems.length} problem(s):`);
  problems.forEach((p) => console.log('  - ' + p));
  process.exitCode = 1;
} else {
  console.log('\nNo problems found.');
}
