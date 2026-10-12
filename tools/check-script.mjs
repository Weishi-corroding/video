// Checks the narration script for pacing problems before you watch it:
// cues that are too dense to voice in their slot, overlaps, and chapter overruns.
//   node tools/check-script.mjs
import { CUES, CHAPTERS, DURATION } from '../js/script.js';

const MAX_WPS = 2.6; // words per second a typical system voice manages at 1×
let words = 0, problems = 0;
CUES.forEach((c, i) => {
  const w = c.en.split(/\s+/).length;
  words += w;
  const ch = CHAPTERS.find((x) => c.start >= x.start && c.start < x.end);
  const issues = [];
  if (w / (c.end - c.start) > MAX_WPS) issues.push(`too dense (${(w / (c.end - c.start)).toFixed(2)} words/s)`);
  if (i > 0 && c.start < CUES[i - 1].end) issues.push(`overlaps cue ${CUES[i - 1].id}`);
  if (!ch || c.end > ch.end) issues.push('runs past its chapter');
  if (!c.zh) issues.push('missing Chinese subtitle');
  if (issues.length) { problems++; console.log(`${c.id}: ${issues.join(', ')}`); }
});
console.log(`${CUES.length} cues, ${words} words, ${Math.round((words / DURATION) * 60)} wpm average over ${DURATION / 60} min; ${problems} problem(s)`);
process.exitCode = problems ? 1 : 0;
