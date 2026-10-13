// HTML labels anchored to 3D points or to the screen. Each label has an English line and a Chinese line;
// the Chinese line shows when Chinese subtitles are on. Labels are kept inside a safe area that never
// reaches the caption band, and anchored labels are nudged apart when they would overlap.
import * as THREE from 'three';

const KICKERS = {
  fact: ['Finding', '发现'],
  principle: ['Principle', '原理'],
  flaw: null,
  shop: null,
  need: null,
};

const tmp = new THREE.Vector3();

export class Labels {
  constructor(layer, camera) {
    this.layer = layer;
    this.camera = camera;
    this.items = [];
    this.zh = false;
    this.W = 1; this.H = 1;
  }

  /**
   * opts: en, zh, kind, at (Vector3 | Object3D | t => Vector3), lift (world y added to `at`),
   * screen: [x, y] as fractions, anchor: [ax, ay] fractions of the label box (default bottom-centre for 3D, centre for screen),
   * vis: t => 0..1, kicker: [en, zh], html: extra markup, cls: extra classes, stem: draw a pointer line.
   */
  add(opts) {
    const el = document.createElement('div');
    el.className = `lbl ${opts.kind || 'tag'} ${opts.cls || ''}`;
    const L = { ...opts, el, w: 0, h: 0, dirty: true, shown: false };
    L.anchor = opts.anchor || (opts.screen ? [0.5, 0.5] : [0.5, 1]);
    L.stem = opts.stem ?? !opts.screen;
    if (L.stem) el.classList.add('stem');
    this.fill(L);
    el.style.opacity = '0';
    this.layer.appendChild(el);
    L.set = (en, zh) => {
      if (en === L.en && zh === L.zh) return;
      L.en = en; L.zh = zh; this.fill(L); L.dirty = true;
    };
    this.items.push(L);
    return L;
  }

  fill(L) {
    const k = L.kicker || KICKERS[L.kind];
    let h = '';
    if (k) h += `<div class="kick"><span class="en">${k[0]}</span><span class="zh">${k[1]}</span></div>`;
    if (L.en) h += `<div class="en">${L.en}</div>`;
    if (L.zh) h += `<div class="zh">${L.zh}</div>`;
    if (L.html) h += L.html;
    L.el.innerHTML = h;
  }

  setZh(on) {
    this.zh = on;
    this.layer.classList.toggle('zh-on', on);
    for (const L of this.items) L.dirty = true;
  }

  resize(W, H) {
    this.W = W; this.H = H;
    for (const L of this.items) L.dirty = true;
  }

  update(t, safe) {
    const { W, H } = this;
    const placed = [];
    for (const L of this.items) {
      const o = L.vis ? L.vis(t) : 1;
      if (o <= 0.003) {
        if (L.shown) { L.el.style.opacity = '0'; L.el.style.visibility = 'hidden'; L.shown = false; }
        continue;
      }
      if (!L.shown) { L.el.style.visibility = 'visible'; L.shown = true; }
      if (L.dirty) { L.w = L.el.offsetWidth; L.h = L.el.offsetHeight; L.dirty = false; }

      let x, y, behind = false;
      if (L.screen) {
        x = L.screen[0] * W; y = L.screen[1] * (safe.bottom - safe.top) + safe.top;
      } else {
        const at = typeof L.at === 'function' ? L.at(t) : L.at.isObject3D ? L.at.getWorldPosition(tmp) : L.at;
        tmp.copy(at);
        if (L.lift) tmp.y += L.lift;
        tmp.project(this.camera);
        behind = tmp.z > 1;
        x = (tmp.x + 1) / 2 * W;
        y = (1 - tmp.y) / 2 * H;
      }
      if (behind) { L.el.style.opacity = '0'; continue; }
      const off = L.offset || [0, 0];
      const stemH = L.stem ? 16 : 0;
      let left = x + off[0] - L.w * L.anchor[0];
      let top = y + off[1] - L.h * L.anchor[1] - stemH;
      const box = { L, left, top, w: L.w, h: L.h, ax: x, ay: y };
      if (!L.screen) {
        // Nudge upward past any anchored label already placed that overlaps.
        for (let pass = 0; pass < 3; pass++) {
          let moved = false;
          for (const b of placed) {
            if (box.left < b.left + b.w + 6 && box.left + box.w + 6 > b.left && box.top < b.top + b.h + 6 && box.top + box.h + 6 > b.top) {
              box.top = b.top - box.h - 8;
              moved = true;
            }
          }
          if (!moved) break;
        }
      }
      // Keep inside the safe area.
      const cl = Math.min(Math.max(box.left, safe.left), W - safe.right - box.w);
      const ct = Math.min(Math.max(box.top, safe.top), safe.bottom - box.h);
      const shifted = Math.abs(cl - left) > 2 || Math.abs(ct - top) > 2;
      box.left = cl; box.top = ct;
      if (!L.screen) placed.push(box);
      L.el.classList.toggle('free', shifted);
      if (L.stem && !shifted) {
        // Point the stem at the anchor even when the label was not moved.
        L.el.style.setProperty('--stem-x', `${Math.round(Math.min(Math.max(x - box.left, 10), box.w - 10))}px`);
      }
      const rise = (1 - o) * 6;
      L.el.style.transform = `translate3d(${Math.round(box.left)}px, ${Math.round(box.top + rise)}px, 0)`;
      L.el.style.opacity = o.toFixed(3);
    }
  }
}
