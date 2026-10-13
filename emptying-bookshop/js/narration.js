// Narration through the browser's speech synthesis, one short utterance per cue.
const PREFERRED = [/Google UK English Female/i, /Google US English/i, /Samantha/i, /Daniel/i, /Microsoft (Aria|Jenny|Guy|Libby|Ryan|Sonia)/i, /Karen/i, /Moira/i];

export class Narrator {
  constructor() {
    this.synth = window.speechSynthesis || null;
    this.voices = [];
    this.voice = null;
    this.speaking = null;   // id of the cue being spoken
    this.onvoices = null;
    if (!this.synth) return;
    const load = () => {
      this.voices = this.synth.getVoices().filter((v) => /^en([-_]|$)/i.test(v.lang));
      if (!this.voice || !this.voices.includes(this.voice)) this.voice = this.pickDefault();
      this.onvoices?.();
    };
    load();
    this.synth.addEventListener?.('voiceschanged', load);
  }

  get available() { return !!this.synth && this.voices.length > 0; }

  pickDefault(savedName) {
    if (savedName) {
      const v = this.voices.find((x) => x.name === savedName);
      if (v) return v;
    }
    for (const re of PREFERRED) {
      const v = this.voices.find((x) => re.test(x.name));
      if (v) return v;
    }
    return this.voices.find((v) => v.localService) || this.voices[0] || null;
  }

  setVoice(name) {
    const v = this.voices.find((x) => x.name === name);
    if (v) this.voice = v;
  }

  speak(cue, rate = 1) {
    if (!this.available) return;
    this.cancel();
    const u = new SpeechSynthesisUtterance(cue.en.replace(/[‘’]/g, "'"));
    u.voice = this.voice;
    u.lang = this.voice?.lang || 'en-GB';
    u.rate = Math.min(2, Math.max(0.5, rate));
    u.pitch = 1;
    const id = cue.id;
    u.onend = u.onerror = () => { if (this.speaking === id) this.speaking = null; };
    this.speaking = id;
    // Chrome sometimes ignores speak() right after cancel(); a tick later is reliable.
    setTimeout(() => { if (this.speaking === id) this.synth.speak(u); }, 0);
  }

  cancel() {
    this.speaking = null;
    if (this.synth && (this.synth.speaking || this.synth.pending)) this.synth.cancel();
  }

  pause() { this.synth?.pause(); }
  resume() { this.synth?.resume(); }
}
