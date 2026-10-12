// Master clock. Everything on screen is a pure function of `time`, so seeking
// is just setting the number.
export class Timeline extends EventTarget {
  constructor(duration) {
    super();
    this.duration = duration;
    this.time = 0;
    this.playing = false;
    this.speed = 1;
  }

  emit(type) { this.dispatchEvent(new Event(type)); }

  play() {
    if (this.time >= this.duration - 0.05) this.time = 0;
    if (this.playing) return;
    this.playing = true;
    this.emit('play');
  }

  pause() {
    if (!this.playing) return;
    this.playing = false;
    this.emit('pause');
  }

  toggle() { this.playing ? this.pause() : this.play(); }

  seek(t) {
    this.time = Math.min(Math.max(t, 0), this.duration);
    this.emit('seek');
  }

  setSpeed(s) {
    this.speed = s;
    this.emit('speed');
  }

  /** Advance by real seconds; `limit(prev, next, dt)` may hold the clock back. */
  tick(dt, limit) {
    if (!this.playing) return;
    let next = this.time + dt * this.speed;
    if (limit) next = limit(this.time, next, dt);
    if (next >= this.duration) {
      this.time = this.duration;
      this.playing = false;
      this.emit('pause');
      this.emit('ended');
    } else {
      this.time = next;
    }
  }
}
