// Player chrome: seek bar split by chapter, menus, toggles, keyboard, captions, start screen.
const $ = (id) => document.getElementById(id);
const fmt = (s) => `${Math.floor(s / 60)}:${String(Math.floor(s % 60)).padStart(2, '0')}`;
const SPEEDS = [0.5, 0.75, 1, 1.25, 1.5, 2];

export function createUI({ timeline, narrator, chapters, duration, state, onChange }) {
  const player = $('player');
  const seek = $('seek'), segs = $('segs'), knob = $('knob'), tip = $('tip');
  const capBox = document.querySelector('.captions .cap');
  const capEn = document.querySelector('.cap-en'), capZh = document.querySelector('.cap-zh');

  // ----- seek bar -----
  const fills = chapters.map((ch) => {
    const seg = document.createElement('div');
    seg.className = 'seg';
    seg.style.flex = `${ch.e - ch.s} 0 0`;
    const fill = document.createElement('div');
    fill.className = 'fill';
    seg.appendChild(fill);
    segs.appendChild(seg);
    return fill;
  });

  // The segments have gaps, so map pointer x to time through the segments themselves.
  const xToTime = (clientX) => {
    const rects = [...segs.children].map((s) => s.getBoundingClientRect());
    for (let i = 0; i < rects.length; i++) {
      const r = rects[i], ch = chapters[i];
      const nextLeft = i + 1 < rects.length ? rects[i + 1].left : Infinity;
      if (clientX < nextLeft || i === rects.length - 1) {
        const f = Math.min(Math.max((clientX - r.left) / r.width, 0), 1);
        return ch.s + f * (ch.e - ch.s);
      }
    }
    return 0;
  };
  const timeToX = (t) => {
    const i = chapters.findIndex((ch) => t < ch.e) ?? chapters.length - 1;
    const k = i < 0 ? chapters.length - 1 : i;
    const r = segs.children[k].getBoundingClientRect(), base = segs.getBoundingClientRect();
    const ch = chapters[k];
    return r.left - base.left + Math.min(Math.max((t - ch.s) / (ch.e - ch.s), 0), 1) * r.width;
  };

  let dragging = false, wasPlaying = false;
  seek.addEventListener('pointerdown', (e) => {
    dragging = true; wasPlaying = timeline.playing;
    seek.setPointerCapture(e.pointerId);
    seek.classList.add('drag');
    timeline.pause();
    timeline.seek(xToTime(e.clientX));
  });
  seek.addEventListener('pointermove', (e) => {
    const t = xToTime(e.clientX);
    const ch = chapters.find((c) => t >= c.s && t < c.e) || chapters[chapters.length - 1];
    tip.textContent = `${fmt(t)} · ${ch.n}. ${ch.title}`;
    tip.style.left = `${e.clientX - seek.getBoundingClientRect().left}px`;
    if (dragging) timeline.seek(t);
  });
  const endDrag = () => {
    if (!dragging) return;
    dragging = false;
    seek.classList.remove('drag');
    if (wasPlaying) timeline.play();
  };
  seek.addEventListener('pointerup', endDrag);
  seek.addEventListener('pointercancel', endDrag);

  // ----- buttons and menus -----
  $('playBtn').addEventListener('click', () => timeline.toggle());
  $('stage').addEventListener('click', () => { closeMenus(); timeline.toggle(); });
  $('labels').addEventListener('click', () => { closeMenus(); timeline.toggle(); });

  const menus = ['chapMenu', 'speedMenu', 'voiceMenu'].map($);
  const closeMenus = () => menus.forEach((m) => m.classList.remove('open'));
  const toggleMenu = (m) => { const open = !m.classList.contains('open'); closeMenus(); if (open) m.classList.add('open'); };
  $('chapBtn').addEventListener('click', (e) => { e.stopPropagation(); renderChapters(); toggleMenu($('chapMenu')); });
  $('speedBtn').addEventListener('click', (e) => { e.stopPropagation(); renderSpeeds(); toggleMenu($('speedMenu')); });
  $('voiceCaret').addEventListener('click', (e) => { e.stopPropagation(); renderVoices(); toggleMenu($('voiceMenu')); });
  document.addEventListener('click', (e) => { if (!e.target.closest('.menu-wrap')) closeMenus(); });

  function renderChapters() {
    const m = $('chapMenu');
    m.innerHTML = '';
    const cur = chapterIndex(timeline.time);
    chapters.forEach((ch, i) => {
      const b = document.createElement('button');
      b.className = i === cur ? 'on' : '';
      b.innerHTML = `<span class="t">${fmt(ch.s)}</span><span>${ch.n}. ${ch.title}</span><span class="zh" lang="zh-Hans">${ch.zh}</span>`;
      b.addEventListener('click', () => { timeline.seek(ch.s); closeMenus(); });
      m.appendChild(b);
    });
  }
  function renderSpeeds() {
    const m = $('speedMenu');
    m.innerHTML = '';
    for (const s of SPEEDS) {
      const b = document.createElement('button');
      b.className = s === timeline.rate ? 'on' : '';
      b.textContent = `${s}×`;
      b.addEventListener('click', () => { setRate(s); closeMenus(); });
      m.appendChild(b);
    }
  }
  function renderVoices() {
    const m = $('voiceMenu');
    m.innerHTML = '';
    if (!narrator.available) {
      m.innerHTML = '<div class="empty">No English system voice was found in this browser. The captions carry the full script.</div>';
      return;
    }
    for (const v of narrator.voices) {
      const b = document.createElement('button');
      b.className = v === narrator.voice ? 'on' : '';
      b.innerHTML = `<span>${v.name}</span><span class="zh">${v.lang}</span>`;
      b.addEventListener('click', () => {
        narrator.setVoice(v.name);
        state.voiceName = v.name;
        if (!state.narr) state.narr = true;
        closeMenus();
        onChange('voice');
        sync();
      });
      m.appendChild(b);
    }
  }

  const setRate = (r) => { timeline.setRate(r); state.rate = r; onChange('rate'); sync(); };
  $('ccBtn').addEventListener('click', () => { state.cc = !state.cc; onChange('cc'); sync(); });
  $('zhBtn').addEventListener('click', () => { state.zh = !state.zh; onChange('zh'); sync(); });
  $('voiceBtn').addEventListener('click', () => toggleNarration());
  $('fsBtn').addEventListener('click', () => toggleFullscreen());

  function toggleNarration() {
    if (!narrator.available) return;
    state.narr = !state.narr;
    onChange('narr');
    sync();
  }
  function toggleFullscreen() {
    if (document.fullscreenElement) document.exitFullscreen?.();
    else player.requestFullscreen?.().catch(() => {});
  }

  // ----- keyboard -----
  const chapterIndex = (t) => { let i = 0; chapters.forEach((ch, k) => { if (t >= ch.s - 0.01) i = k; }); return i; };
  window.addEventListener('keydown', (e) => {
    if (e.target.closest && e.target.closest('input, textarea')) return;
    if (e.metaKey || e.ctrlKey || e.altKey) return;
    const k = e.key;
    let handled = true;
    if (k === ' ' || k === 'k' || k === 'K') {
      if (!$('start').classList.contains('hidden')) { $('startVoice').click(); }
      else timeline.toggle();
    }
    else if (k === 'ArrowRight') timeline.seek(timeline.time + (e.shiftKey ? 15 : 5));
    else if (k === 'ArrowLeft') timeline.seek(timeline.time - (e.shiftKey ? 15 : 5));
    else if (k === 'ArrowDown') { const i = chapterIndex(timeline.time); if (i + 1 < chapters.length) timeline.seek(chapters[i + 1].s); }
    else if (k === 'ArrowUp') {
      const i = chapterIndex(timeline.time);
      // Within the first 3 s of a chapter, go to the previous one; otherwise to this chapter's start.
      timeline.seek(timeline.time - chapters[i].s < 3 && i > 0 ? chapters[i - 1].s : chapters[i].s);
    }
    else if (/^[1-9]$/.test(k) && +k <= chapters.length) timeline.seek(chapters[+k - 1].s);
    else if (k === 'Home') timeline.seek(0);
    else if (k === 'End') timeline.seek(duration - 0.01);
    else if (k === ',' || k === '<') { const i = SPEEDS.indexOf(timeline.rate); setRate(SPEEDS[Math.max(0, i - 1)]); }
    else if (k === '.' || k === '>') { const i = SPEEDS.indexOf(timeline.rate); setRate(SPEEDS[Math.min(SPEEDS.length - 1, i + 1)]); }
    else if (k === 'c' || k === 'C') $('ccBtn').click();
    else if (k === 'z' || k === 'Z') $('zhBtn').click();
    else if (k === 'n' || k === 'N') toggleNarration();
    else if (k === 'f' || k === 'F') toggleFullscreen();
    else if (k === 'Escape') closeMenus();
    else handled = false;
    if (handled) { e.preventDefault(); wake(); }
  });

  // ----- auto-hide -----
  let idleTimer = 0;
  const wake = () => {
    player.classList.remove('idle');
    clearTimeout(idleTimer);
    idleTimer = setTimeout(() => { if (timeline.playing && !dragging && !menus.some((m) => m.classList.contains('open'))) player.classList.add('idle'); }, 2600);
  };
  player.addEventListener('pointermove', wake);
  timeline.on((type) => { if (type === 'pause' || type === 'end') player.classList.remove('idle'); else wake(); sync(); });

  // ----- state → buttons -----
  function sync() {
    $('ccBtn').setAttribute('aria-pressed', String(state.cc));
    $('zhBtn').setAttribute('aria-pressed', String(state.zh));
    $('voiceBtn').setAttribute('aria-pressed', String(state.narr && narrator.available));
    $('voiceBtn').disabled = !narrator.available;
    $('voiceCaret').disabled = false;
    $('speedBtn').textContent = `${timeline.rate}×`;
    player.classList.toggle('playing', timeline.playing);
    $('playBtn').setAttribute('aria-label', timeline.playing ? 'Pause (Space)' : 'Play (Space)');
    $('voiceNote').textContent = narrator.available ? '' : 'No English system voice was found, so the film will play with captions only.';
  }
  narrator.onvoices = sync;

  let lastCap = null;
  function update(t, capCue) {
    fills.forEach((f, i) => {
      const ch = chapters[i];
      f.style.width = `${Math.min(Math.max((t - ch.s) / (ch.e - ch.s), 0), 1) * 100}%`;
    });
    knob.style.left = `${timeToX(t)}px`;
    $('time').textContent = `${fmt(t)} / ${fmt(duration)}`;
    const ch = chapters[chapterIndex(t)];
    $('chname').textContent = `${ch.n}. ${ch.title}`;
    const key = capCue ? `${capCue.id}|${state.cc}|${state.zh}` : null;
    if (key !== lastCap) {
      lastCap = key;
      capEn.textContent = capCue && state.cc ? capCue.en : '';
      capZh.textContent = capCue && state.zh ? capCue.zh : '';
      capBox.classList.toggle('on', !!capCue && (state.cc || state.zh));
    }
  }

  // ----- start screen -----
  function showStart(onStart) {
    $('start').classList.remove('hidden');
    $('startZh').checked = state.zh;
    $('startZh').addEventListener('change', () => { state.zh = $('startZh').checked; onChange('zh'); sync(); });
    $('startVoice').disabled = !narrator.available;
    const go = (narr) => {
      $('start').classList.add('hidden');
      state.narr = narr && narrator.available;
      onChange('narr');
      sync();
      onStart();
    };
    $('startVoice').addEventListener('click', () => go(true));
    $('startMuted').addEventListener('click', () => go(false));
    // Voices can arrive late; re-enable the narration button when they do.
    const prev = narrator.onvoices;
    narrator.onvoices = () => { prev?.(); $('startVoice').disabled = !narrator.available; };
  }

  sync();
  return { update, sync, showStart, captionTop: () => capBox.getBoundingClientRect().top };
}
