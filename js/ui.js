// Player chrome: seek bar with chapter segments, chapter/speed/voice menus,
// caption toggles, keyboard shortcuts, captions, and the start screen.
import { CHAPTERS, CUES, DURATION, chapterAt } from './script.js';

const SPEEDS = [0.5, 0.75, 1, 1.25, 1.5, 2];
const $ = (s) => document.querySelector(s);
const fmt = (t) => {
  const s = Math.max(0, Math.floor(t + 1e-6));
  return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
};
const pad2 = (n) => String(n).padStart(2, '0');
const END_CARD = CUES.find((c) => c.endCard).start; // the HUD clears for the end card
/** Captions sit in an inline span, so each wrapped line gets its own snug background. */
function setCaption(p, text) {
  p.textContent = '';
  if (!text) return;
  const span = document.createElement('span');
  span.textContent = text;
  p.appendChild(span);
}

function loadSettings() {
  try { return JSON.parse(localStorage.getItem('essay.settings')) || {}; } catch { return {}; }
}

export class UI {
  constructor({ timeline, narrator, params }) {
    this.tl = timeline;
    this.nr = narrator;
    const saved = loadSettings();
    this.settings = {
      cc: params.has('cc') ? params.get('cc') !== '0' : saved.cc ?? true,
      zh: params.has('zh') ? params.get('zh') !== '0' : saved.zh ?? false,
      voice: saved.voice ?? true,
      speed: SPEEDS.includes(saved.speed) ? saved.speed : 1,
    };
    this.tl.setSpeed(this.settings.speed);
    this.capIndex = -2;
    this.chapterIndex = -1;
    this.dragging = false;
    this.idleTimer = 0;

    this.buildSeek();
    this.buildMenus();
    this.bindButtons();
    this.bindKeys();
    this.bindIdle();
    this.buildStart();
    this.applySettings();

    this.tl.addEventListener('play', () => { document.body.classList.add('playing'); $('#btn-play').setAttribute('aria-label', 'Pause'); this.poke(); });
    this.tl.addEventListener('pause', () => { document.body.classList.remove('playing'); $('#btn-play').setAttribute('aria-label', 'Play'); this.wake(); });
    this.nr.addEventListener('voices', () => this.refreshVoice());
    this.nr.addEventListener('state', () => this.refreshVoice());
    this.refreshVoice();
    setTimeout(() => this.refreshVoice(), 5200);
  }

  save() { try { localStorage.setItem('essay.settings', JSON.stringify(this.settings)); } catch {} }

  // ---------- seek bar ----------
  buildSeek() {
    const seek = (this.seekEl = $('#seek'));
    const segWrap = seek.querySelector('.segments');
    this.segs = CHAPTERS.map((ch) => {
      const s = document.createElement('div');
      s.className = 'seg';
      s.style.flex = `${ch.end - ch.start} 1 0`;
      s.innerHTML = '<i></i>';
      segWrap.appendChild(s);
      return { el: s, fill: s.firstChild, ch };
    });
    this.thumb = seek.querySelector('.thumb');
    this.tip = seek.querySelector('.tip');

    const timeAtX = (clientX) => {
      for (let i = 0; i < this.segs.length; i++) {
        const r = this.segs[i].el.getBoundingClientRect();
        const last = i === this.segs.length - 1;
        if (clientX <= r.right + 1.5 || last) {
          const f = Math.min(1, Math.max(0, (clientX - r.left) / r.width));
          const ch = this.segs[i].ch;
          return ch.start + f * (ch.end - ch.start);
        }
      }
      return 0;
    };
    const showTip = (clientX) => {
      const t = timeAtX(clientX);
      const sr = seek.getBoundingClientRect();
      const ch = CHAPTERS[chapterAt(t)];
      this.tip.querySelector('b').textContent = fmt(t);
      this.tip.querySelector('span').textContent = `${pad2(ch.n)} ${ch.title}`;
      const x = Math.min(sr.width - 120, Math.max(120, clientX - sr.left));
      this.tip.style.left = `${x}px`;
      return t;
    };
    seek.addEventListener('pointermove', (e) => {
      const t = showTip(e.clientX);
      if (this.dragging) this.tl.seek(t);
    });
    seek.addEventListener('pointerdown', (e) => {
      this.dragging = true;
      seek.classList.add('dragging');
      seek.setPointerCapture(e.pointerId);
      this.wasPlaying = this.tl.playing;
      if (this.wasPlaying) this.tl.pause();
      this.tl.seek(showTip(e.clientX));
    });
    const end = () => {
      if (!this.dragging) return;
      this.dragging = false;
      seek.classList.remove('dragging');
      if (this.wasPlaying) this.tl.play();
    };
    seek.addEventListener('pointerup', end);
    seek.addEventListener('pointercancel', end);
    seek.addEventListener('keydown', (e) => {
      if (e.key === 'Home') { this.tl.seek(0); e.preventDefault(); }
      if (e.key === 'End') { this.tl.seek(DURATION); e.preventDefault(); }
    });
    this.timeAtX = timeAtX;
  }

  // ---------- menus ----------
  buildMenus() {
    const chMenu = $('#menu-chapters');
    this.chItems = CHAPTERS.map((ch, i) => {
      const li = document.createElement('li');
      li.innerHTML = `<button role="menuitemradio" aria-checked="false"><span><span class="n">${pad2(ch.n)}</span>${ch.title}<span class="zh-line">${ch.zh}</span></span><span class="d">${fmt(ch.start)}</span></button>`;
      li.firstChild.addEventListener('click', () => { this.tl.seek(ch.start); this.closeMenus(); });
      chMenu.appendChild(li);
      return li.firstChild;
    });
    const spMenu = $('#menu-speed');
    this.spItems = SPEEDS.map((s) => {
      const li = document.createElement('li');
      li.innerHTML = `<button role="menuitemradio" aria-checked="false">${s === 1 ? 'Normal' : s + '×'}</button>`;
      li.firstChild.addEventListener('click', () => { this.setSpeed(s); this.closeMenus(); });
      spMenu.appendChild(li);
      return { s, el: li.firstChild };
    });
    // a small caret next to the Voice toggle opens the list of English voices
    const caret = document.createElement('button');
    caret.className = 'icon caret';
    caret.id = 'btn-voice-menu';
    caret.setAttribute('aria-label', 'Choose narration voice');
    caret.setAttribute('aria-haspopup', 'true');
    caret.innerHTML = '<svg viewBox="0 0 24 24"><path d="M7 10l5 5 5-5z"/></svg>';
    caret.style.minWidth = '22px';
    caret.style.padding = '0 2px';
    $('#btn-voice').after(caret);

    const toggle = (btn, menu) => {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        const open = !menu.classList.contains('open');
        this.closeMenus();
        if (open) { menu.classList.add('open'); btn.setAttribute('aria-expanded', 'true'); }
      });
    };
    toggle($('#btn-chapters'), chMenu);
    toggle($('#btn-speed'), spMenu);
    toggle(caret, $('#menu-voice'));
    document.addEventListener('click', (e) => { if (!e.target.closest('.menu')) this.closeMenus(); });
  }

  closeMenus() {
    document.querySelectorAll('.menu.open').forEach((m) => m.classList.remove('open'));
    document.querySelectorAll('[aria-expanded="true"]').forEach((b) => b.setAttribute('aria-expanded', 'false'));
  }

  refreshVoice() {
    const btn = $('#btn-voice');
    const avail = this.nr.available;
    btn.disabled = !avail;
    btn.title = avail ? 'English narration (N)' : 'No English system voice found: captions only';
    btn.setAttribute('aria-pressed', String(this.nr.enabled));
    $('#btn-voice-menu').disabled = !avail;
    // voices can arrive after the viewer has already pressed Play
    if (avail && this.settings.voice && !this.nr.enabled && !document.body.classList.contains('at-start')) {
      this.nr.setEnabled(true);
      return;
    }
    const startVoice = $('#start-voice');
    startVoice.disabled = !avail;
    startVoice.checked = avail && this.settings.voice;
    const stillLooking = this.nr.supported && performance.now() < 5000;
    $('#voice-note').textContent = avail
      ? `Narration voice: ${this.nr.voice?.name || 'system default'}. You can change it from the player controls.`
      : stillLooking
        ? 'Looking for an English voice on this device…'
        : this.nr.supported
        ? 'No English voice was found on this device, so narration is off. Captions carry the full script.'
        : 'This browser has no speech synthesis, so narration is off. Captions carry the full script.';
    const menu = $('#menu-voice');
    menu.innerHTML = '';
    if (!avail) {
      menu.innerHTML = '<li class="note">No English voice available.</li>';
      return;
    }
    this.nr.voices.forEach((v) => {
      const li = document.createElement('li');
      const b = document.createElement('button');
      b.setAttribute('role', 'menuitemradio');
      b.setAttribute('aria-checked', String(v === this.nr.voice));
      b.innerHTML = `<span>${v.name}</span><span class="d">${v.lang}</span>`;
      b.addEventListener('click', () => { this.nr.setVoice(v.name); this.refreshVoice(); this.closeMenus(); });
      li.appendChild(b);
      menu.appendChild(li);
    });
  }

  // ---------- buttons & settings ----------
  bindButtons() {
    $('#btn-play').addEventListener('click', () => this.tl.toggle());
    $('#btn-prev').addEventListener('click', () => this.prevChapter());
    $('#btn-next').addEventListener('click', () => this.nextChapter());
    $('#btn-cc').addEventListener('click', () => this.setCC(!this.settings.cc));
    $('#btn-zh').addEventListener('click', () => this.setZH(!this.settings.zh));
    $('#btn-voice').addEventListener('click', () => this.setVoice(!this.nr.enabled));
    $('#btn-fs').addEventListener('click', () => this.toggleFullscreen());
    // clicking the picture toggles playback
    $('#gl').addEventListener('click', () => { if (!document.body.classList.contains('at-start')) this.tl.toggle(); });
  }

  setSpeed(s) {
    this.settings.speed = s;
    this.tl.setSpeed(s);
    this.save();
    this.applySettings();
  }
  setCC(on) { this.settings.cc = on; this.save(); this.applySettings(); }
  setZH(on) { this.settings.zh = on; this.save(); this.applySettings(); }
  setVoice(on) {
    this.settings.voice = on;
    this.save();
    this.nr.setEnabled(on);
    this.dispatchVoiceToggle?.(on);
  }

  applySettings() {
    document.body.classList.toggle('zh', this.settings.zh);
    document.body.classList.toggle('no-cc', !this.settings.cc);
    $('#btn-cc').setAttribute('aria-pressed', String(this.settings.cc));
    $('#btn-zh').setAttribute('aria-pressed', String(this.settings.zh));
    $('#start-zh').checked = this.settings.zh;
    const s = this.settings.speed;
    $('#btn-speed').textContent = `${s}×`;
    this.spItems.forEach((it) => it.el.setAttribute('aria-checked', String(it.s === s)));
  }

  prevChapter() {
    const t = this.tl.time;
    const i = chapterAt(t);
    const target = t - CHAPTERS[i].start > 2.5 || i === 0 ? CHAPTERS[i].start : CHAPTERS[i - 1].start;
    this.tl.seek(target);
  }
  nextChapter() {
    const i = chapterAt(this.tl.time);
    if (i < CHAPTERS.length - 1) this.tl.seek(CHAPTERS[i + 1].start);
  }

  toggleFullscreen() {
    const el = document.documentElement;
    if (document.fullscreenElement) document.exitFullscreen?.();
    else (el.requestFullscreen || el.webkitRequestFullscreen)?.call(el);
  }

  bindKeys() {
    window.addEventListener('keydown', (e) => {
      if (e.target.closest?.('input, select, textarea') || e.metaKey || e.ctrlKey || e.altKey) return;
      const atStart = document.body.classList.contains('at-start');
      let handled = true;
      switch (e.key) {
        case ' ': case 'k': case 'K':
          if (atStart) this.start(); else this.tl.toggle();
          break;
        case 'ArrowLeft': this.tl.seek(this.tl.time - (e.shiftKey ? 15 : 5)); break;
        case 'ArrowRight': this.tl.seek(this.tl.time + (e.shiftKey ? 15 : 5)); break;
        case 'ArrowUp': case 'PageUp': this.prevChapter(); break;
        case 'ArrowDown': case 'PageDown': this.nextChapter(); break;
        case 'c': case 'C': this.setCC(!this.settings.cc); break;
        case 'z': case 'Z': this.setZH(!this.settings.zh); break;
        case 'n': case 'N': if (this.nr.available) this.setVoice(!this.nr.enabled); break;
        case 'f': case 'F': this.toggleFullscreen(); break;
        case '<': case ',': this.stepSpeed(-1); break;
        case '>': case '.': this.stepSpeed(1); break;
        case 'Escape': this.closeMenus(); break;
        default:
          if (/^[1-7]$/.test(e.key)) { this.tl.seek(CHAPTERS[+e.key - 1].start); if (atStart) this.start(false); }
          else handled = false;
      }
      if (handled) { e.preventDefault(); this.poke(); }
    });
  }
  stepSpeed(d) {
    const i = SPEEDS.indexOf(this.settings.speed);
    this.setSpeed(SPEEDS[Math.min(SPEEDS.length - 1, Math.max(0, i + d))]);
  }

  // ---------- idle hiding ----------
  bindIdle() {
    const onMove = () => this.poke();
    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerdown', onMove);
    window.addEventListener('touchstart', onMove, { passive: true });
  }
  wake() {
    document.body.classList.remove('idle');
    clearTimeout(this.idleTimer);
  }
  poke() {
    this.wake();
    this.idleTimer = setTimeout(() => {
      if (this.tl.playing && !document.querySelector('.menu.open') && !this.dragging && !$('#controls').matches(':hover')) {
        document.body.classList.add('idle');
      }
    }, 2600);
  }

  // ---------- start screen ----------
  buildStart() {
    document.body.classList.add('at-start');
    const toc = $('#toc');
    CHAPTERS.forEach((ch) => {
      const li = document.createElement('li');
      li.innerHTML = `<button><span class="n">${pad2(ch.n)}</span><span class="t">${ch.title}<span class="zh-line">${ch.zh}</span></span><span class="d">${fmt(ch.start)}</span></button>`;
      li.firstChild.addEventListener('click', () => { this.tl.seek(ch.start); this.start(); });
      toc.appendChild(li);
    });
    $('#start-play').addEventListener('click', () => this.start());
    $('#start-zh').addEventListener('change', (e) => this.setZH(e.target.checked));
    $('#start-voice').addEventListener('change', (e) => { this.settings.voice = e.target.checked; this.save(); });
  }

  /** Leave the start screen. The click is the user gesture speech synthesis needs. */
  start(play = true) {
    if (!document.body.classList.contains('at-start')) return;
    document.body.classList.remove('at-start');
    $('#start').classList.add('gone');
    setTimeout(() => ($('#start').hidden = true), 700);
    this.nr.setEnabled(this.settings.voice);
    if (play) this.tl.play();
    $('#btn-play').focus({ preventScroll: true });
  }

  skipStart() {
    document.body.classList.remove('at-start');
    $('#start').hidden = true;
  }

  // ---------- per-frame ----------
  update(t) {
    const ci = chapterAt(t);
    $('#time').textContent = `${fmt(t)} / ${fmt(DURATION)}`;
    this.seekEl.setAttribute('aria-valuenow', String(Math.round(t)));
    this.seekEl.setAttribute('aria-valuetext', `${fmt(t)}, chapter ${ci + 1}: ${CHAPTERS[ci].title}`);
    const sr = this.seekEl.getBoundingClientRect();
    this.segs.forEach((s, i) => {
      const f = Math.min(1, Math.max(0, (t - s.ch.start) / (s.ch.end - s.ch.start)));
      s.fill.style.transform = `scaleX(${f})`;
      s.el.classList.toggle('current', i === ci);
      if (i === ci) {
        const r = s.el.getBoundingClientRect();
        this.thumb.style.left = `${r.left - sr.left + f * r.width}px`;
      }
    });
    const chip = $('#chapter-chip');
    const intro = t - CHAPTERS[ci].start < 3.4 && !document.body.classList.contains('at-start');
    if (chip.classList.contains('intro') !== intro) chip.classList.toggle('intro', intro);
    if (ci !== this.chapterIndex) {
      this.chapterIndex = ci;
      const ch = CHAPTERS[ci];
      $('#chapter-chip .num').textContent = pad2(ch.n);
      $('#chapter-chip .title').textContent = ch.title;
      $('#chapter-chip .zh-line').textContent = ch.zh;
      this.chItems.forEach((b, i) => b.setAttribute('aria-checked', String(i === ci)));
    }
    // captions: the most recent cue, held briefly after it ends
    let idx = -1;
    for (let i = 0; i < CUES.length && CUES[i].start <= t; i++) idx = i;
    if (idx >= 0) {
      const cue = CUES[idx], next = CUES[idx + 1];
      if (!(t < cue.end + 0.7 && (!next || t < next.start))) idx = -1;
    }
    if (idx !== this.capIndex) {
      this.capIndex = idx;
      // non-breaking hyphens, so balanced lines never split "AI-allowed" after the dash
      setCaption($('#captions .en'), idx >= 0 ? CUES[idx].en.replace(/(\w)-(\w)/g, '$1‑$2') : '');
      setCaption($('#captions .zh'), idx >= 0 ? CUES[idx].zh : '');
      $('#captions').classList.toggle('on-screen', idx >= 0 && !!CUES[idx].onScreen);
    }
    // the opening card says "dramatization"; the corner badge takes over once it leaves
    document.body.classList.toggle('drama-intro', t < CUES[1].start);
    document.body.classList.toggle('at-end', t >= END_CARD - 0.2);
  }
}
