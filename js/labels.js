// HTML labels anchored to 3D objects (readable typography instead of 3D text),
// plus screen-space overlay cards. Scenes set each label's opacity every frame.
import * as THREE from 'three';

const v = new THREE.Vector3();

export class LabelLayer {
  constructor(anchorRoot, overlayRoot) {
    this.anchorRoot = anchorRoot;
    this.overlayRoot = overlayRoot;
    this.items = [];
    this.zh = false;
  }

  /**
   * opts: { html, zh, cls, anchor: Object3D, dx, dy, align: 'top'|'center'|'left'|'right', chapter, overlay }
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
      set(o) { this.opacity = o; },
    };
    this.items.push(item);
    return item;
  }

  /** Called once per frame after scenes set opacities. `active` maps chapter → opacity multiplier. */
  update(camera, width, height, active) {
    for (const it of this.items) {
      const o = Math.min(1, it.opacity) * (active.get(it.chapter) || 0);
      if (o <= 0.004) {
        if (it.shown) { it.el.style.display = 'none'; it.shown = false; }
        continue;
      }
      if (!it.shown) { it.el.style.display = ''; it.shown = true; }
      it.el.style.opacity = o.toFixed(3);
      if (it.overlay || !it.anchor) continue;
      it.anchor.getWorldPosition(v);
      v.project(camera);
      if (v.z > 1 || v.z < -1) { it.el.style.opacity = '0'; continue; }
      const x = (v.x * 0.5 + 0.5) * width + it.dx;
      const y = (-v.y * 0.5 + 0.5) * height + it.dy;
      const a = it.align;
      const tx = a === 'left' ? '0%' : a === 'right' ? '-100%' : '-50%';
      const ty = a === 'top' ? '-100%' : a === 'below' ? '0%' : '-50%';
      it.el.style.transform = `translate3d(${x.toFixed(1)}px, ${y.toFixed(1)}px, 0) translate(${tx}, ${ty})`;
    }
  }
}
