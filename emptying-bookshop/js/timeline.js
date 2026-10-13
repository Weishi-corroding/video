// The master clock. Everything on screen is computed from `time`, so seeking and speed changes are exact.
export class Timeline {
  constructor(duration) {
    this.duration = duration;
    this.time = 0;
    this.playing = false;
    this.rate = 1;
    this.listeners = new Set();
  }

  on(fn) { this.listeners.add(fn); return () => this.listeners.delete(fn); }
  emit(type) { for (const fn of this.listeners) fn(type, this); }

  play() {
    if (this.time >= this.duration - 0.05) this.time = 0;
    if (!this.playing) { this.playing = true; this.emit('play'); }
  }
  pause() { if (this.playing) { this.playing = false; this.emit('pause'); } }
  toggle() { this.playing ? this.pause() : this.play(); }

  seek(t) {
    this.time = Math.min(Math.max(t, 0), this.duration);
    this.emit('seek');
  }

  setRate(r) { this.rate = r; this.emit('rate'); }

  /** Advance by dt real seconds, never past `limit` (used to wait for narration). */
  advance(dt, limit = Infinity) {
    if (!this.playing) return;
    const next = Math.min(this.time + dt * this.rate, this.duration, Math.max(limit, this.time));
    this.time = next;
    if (this.time >= this.duration) { this.playing = false; this.emit('end'); }
  }
}
