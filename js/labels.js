// HTML labels anchored to 3D objects (readable typography instead of 3D text),
// plus screen-space overlay cards. Scenes set each label's opacity every frame;
// a small layout pass then nudges overlapping labels apart, away from overlay
// cards and captions, and keeps them on screen.
import * as THREE from 'three';

const v = new THREE.Vector3();
const GAP = 6;   // px kept between neighbours
const EDGE = 8;  // px kept from the screen edge
const PASSES = 4;
// alignment → fraction of the label's own size to shift by
const ALIGN = {
  top: [-0.5, -1], below: [-0.5, 0], center: [-0.5, -0.5], left: [0, -0.5], right: [-1, -0.5],
};

/**
 * Overlap of two rects (with GAP) and the move that separates them: along the
 * cheaper axis, leaning vertical because labels are wide and short (and a
 * sideways push often runs into the screen edge).
 */
function separation(ax, ay, aw, ah, bx, by, bw, bh) {
  const ox = Math.min(ax + aw, bx + bw) - Math.max(ax, bx) + GAP;
  const oy = Math.min(ay + ah, by + bh) - Math.max(ay, by) + GAP;
  if (ox <= 0 || oy <= 0) return null;
  if (ox < oy * 0.5) return { ux: bx + bw / 2 >= ax + aw / 2 ? 1 : -1, uy: 0, s: ox };
  return { ux: 0, uy: by + bh / 2 >= ay + ah / 2 ? 1 : -1, s: oy };
}

export class LabelLayer {
  constructor(anchorRoot, overlayRoot, captions) {
    this.anchorRoot = anchorRoot;
    this.overlayRoot = overlayRoot;
    this.captions = captions;
    this.items = [];
    this.zh = false;
  }

  /**
   * opts: { html, zh, cls, anchor: Object3D, dx, dy, align: 'top'|'below'|'center'|'left'|'right', chapter, overlay }
   * Returns a handle with .set(opacity) and .el.
   */
  add(opts) {
    const el = document.createElement('div');
    el.className = `lbl ${opts.cls || 'tag'}` + (opts.overlay ? ' overlay' : '');
    el.innerHTML = opts.html + (opts.zh ? `<span class="zh-line">${opts.zh}</span>` : '');
    el.style.opacity = '0';
    el.style.display = 'none';
    (opts.overlay ? this.overlayRoot : this.anchorRoot).appendChild(el);
    const item = {
      el,
      anchor: opts.anchor || null,
      dx: opts.dx || 0,
      dy: opts.dy || 0,
      align: opts.align || 'top',
      chapter: opts.chapter,
      overlay: !!opts.overlay,
      opacity: 0,
      shown: false,
      measured: '',
      set(o) { this.opacity = o; },
    };
    this.items.push(item);
    return item;
  }

  /** Called once per frame after scenes set opacities. `active` maps chapter → opacity multiplier. */
  update(camera, width, height, active) {
    // sizes only change with the viewport or the subtitle language, so measure on those
    const key = `${width}x${height}${document.body.classList.contains('zh') ? ':zh' : ''}`;
    const boxes = [], blocks = [];
    for (const it of this.items) {
      const o = Math.min(1, it.opacity) * (active.get(it.chapter) || 0);
      if (o <= 0.004) {
        if (it.shown) { it.el.style.display = 'none'; it.shown = false; }
        continue;
      }
      if (!it.shown) { it.el.style.display = ''; it.shown = true; }
      it.el.style.opacity = o.toFixed(3);
      if (it.measured !== key) {
        it.measured = key;
        if (it.overlay) {
          const r = it.el.getBoundingClientRect();
          it.rect = { x: r.left, y: r.top, w: r.width, h: r.height };
        } else {
          it.w = it.el.offsetWidth;
          it.h = it.el.offsetHeight;
        }
      }
      if (it.overlay) { blocks.push({ ...it.rect, o }); continue; }
      if (!it.anchor) continue;
      it.anchor.getWorldPosition(v);
      v.project(camera);
      if (v.z > 1 || v.z < -1) { it.el.style.opacity = '0'; continue; }
      const [fx, fy] = ALIGN[it.align] || ALIGN.top;
      boxes.push({
        it, o, w: it.w, h: it.h,
        x: (v.x * 0.5 + 0.5) * width + it.dx + fx * it.w,
        y: (-v.y * 0.5 + 0.5) * height + it.dy + fy * it.h,
      });
    }
    for (const p of this.captions.children) {
      const r = p.firstChild && p.firstChild.getBoundingClientRect && p.firstChild.getBoundingClientRect();
      if (r && r.width > 0 && !this.captions.classList.contains('on-screen')) blocks.push({ x: r.left, y: r.top, w: r.width, h: r.height, o: 1 });
    }

    // relax: the fainter label of a pair gives way, so a label fading in never shoves a settled one
    for (let pass = 0; pass < PASSES; pass++) {
      for (let i = 0; i < boxes.length; i++) {
        const a = boxes[i];
        for (let j = i + 1; j < boxes.length; j++) {
          const b = boxes[j];
          const m = separation(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h);
          if (!m) continue;
          const wa = b.o / (a.o + b.o), wb = a.o / (a.o + b.o);
          a.x -= m.ux * m.s * wa; a.y -= m.uy * m.s * wa;
          b.x += m.ux * m.s * wb; b.y += m.uy * m.s * wb;
        }
        for (const k of blocks) {
          const m = separation(k.x, k.y, k.w, k.h, a.x, a.y, a.w, a.h);
          if (m) { a.x += m.ux * m.s * k.o; a.y += m.uy * m.s * k.o; }
        }
      }
      for (const a of boxes) {
        a.x = a.w > width - 2 * EDGE ? (width - a.w) / 2 : Math.min(Math.max(a.x, EDGE), width - EDGE - a.w);
        a.y = Math.min(Math.max(a.y, EDGE), height - EDGE - a.h);
      }
    }
    for (const a of boxes) a.it.el.style.transform = `translate3d(${a.x.toFixed(1)}px, ${a.y.toFixed(1)}px, 0)`;
  }
}
