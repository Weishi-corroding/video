// Optional English narration through the browser's Speech Synthesis API.
// Each cue is spoken as its own short utterance (long utterances are unreliable
// in some browsers). The player holds the timeline at a cue's end while the
// voice is still talking, so picture and narration stay in step for any voice.
import { CUES } from './script.js';

const PREFERRED = [/natural/i, /neural/i, /premium/i, /enhanced/i, /google us english/i, /google uk english/i, /samantha/i, /aria/i, /jenny/i, /guy/i, /daniel/i, /serena/i, /karen/i, /moira/i];

export class Narrator extends EventTarget {
  constructor() {
    super();
    this.supported = typeof window !== 'undefined' && 'speechSynthesis' in window && 'SpeechSynthesisUtterance' in window;
    this.voices = [];
    this.voice = null;
    this.enabled = false;
    this.cue = -1;
    this.current = null;
    this.startedAt = 0;
    if (!this.supported) return;
    const load = () => this.loadVoices();
    load();
    speechSynthesis.addEventListener?.('voiceschanged', load);
    // some engines populate voices late without firing the event
    let tries = 0;
    const iv = setInterval(() => { load(); if (this.voices.length || ++tries > 20) clearInterval(iv); }, 250);
  }

  get available() { return this.supported && this.voices.length > 0; }

  loadVoices() {
    const all = speechSynthesis.getVoices() || [];
    const en = all.filter((v) => /^en([-_]|$)/i.test(v.lang));
    const score = (v) => {
      let s = 0;
      PREFERRED.forEach((re, i) => { if (re.test(v.name)) s += 30 - i; });
      if (/^en[-_]US/i.test(v.lang)) s += 6;
      else if (/^en[-_]GB/i.test(v.lang)) s += 4;
      if (v.localService) s += 1;
      return s;
    };
    en.sort((a, b) => score(b) - score(a));
    const changed = en.length !== this.voices.length;
    this.voices = en;
    const savedName = localStorage.getItem('essay.voice');
    if (!this.voice || !en.includes(this.voice)) {
      this.voice = en.find((v) => v.name === savedName) || en[0] || null;
    }
    if (changed) this.dispatchEvent(new Event('voices'));
  }

  setVoice(name) {
    const v = this.voices.find((x) => x.name === name);
    if (v) { this.voice = v; localStorage.setItem('essay.voice', name); }
  }

  setEnabled(on) {
    this.enabled = on && this.available;
    if (!this.enabled) this.cancel();
    this.dispatchEvent(new Event('state'));
  }

  speak(i, rate = 1) {
    if (!this.enabled || !this.voice) return;
    speechSynthesis.cancel();
    const u = new SpeechSynthesisUtterance(CUES[i].en);
    u.voice = this.voice;
    u.lang = this.voice.lang;
    u.rate = Math.min(2, Math.max(0.6, rate));
    u.pitch = 1;
    const done = () => { if (this.current === u) { this.current = null; this.cue = -1; } };
    u.onend = done;
    u.onerror = done;
    this.current = u; // keep a reference: some engines drop events for collected utterances
    this.cue = i;
    this.startedAt = performance.now();
    speechSynthesis.speak(u);
  }

  /** True while the current cue is still being voiced. */
  get speaking() {
    if (!this.current) return false;
    const engineBusy = speechSynthesis.speaking || speechSynthesis.pending;
    // allow the engine a moment to start before trusting its flags
    if (!engineBusy && performance.now() - this.startedAt > 1500) { this.current = null; this.cue = -1; return false; }
    return true;
  }

  cancel() {
    this.current = null;
    this.cue = -1;
    if (this.supported) speechSynthesis.cancel();
  }
}
