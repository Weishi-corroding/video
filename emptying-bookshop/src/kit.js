// Shared toolkit: palette, easing, materials, and the small props the film is built from.
import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

// Colour carries meaning, and every coloured label also says what it is:
// teal = a fact Jia found, coral = an assumption or a problem, gold = a named principle,
// blue / violet = the two sides of the shop's contradiction.
export const ROLE = {
  fact: '#4fd1c5',
  flaw: '#ff7a6b',
  principle: '#ffcf5a',
  shopSide: '#7aa2ff',
  needSide: '#d08cff',
};

export const PAL = {
  lawn: 0x8fae7e,
  lawnDark: 0x7b9a6c,
  path: 0xe6dac4,
  plinth: 0x2a3140,
  stone: 0xdccfb6,
  cream: 0xf0e6d2,
  brick: 0xc98e6c,
  roof: 0xa65f48,
  roofGrey: 0x6f7a86,
  wood: 0xb98a5e,
  woodDark: 0x7d5a3c,
  glass: 0xbfd8e6,
  skin: [0xf1d0b5, 0xd9a882, 0xb98563, 0x8d5f43, 0xf5dcc8],
  shirts: [0x5b7fa6, 0xc9785d, 0x6f9a7a, 0xd9b45a, 0x8a6fae, 0x4f6f78, 0xb85c6a, 0xe0ddd2, 0x5e6b8c, 0x9a8b6a],
  apron: 0x2f6b55,
};

// ---------- maths and easing ----------
export const clamp = (x, a = 0, b = 1) => Math.min(b, Math.max(a, x));
export const lerp = (a, b, t) => a + (b - a) * t;
export const smooth = (x) => { x = clamp(x); return x * x * (3 - 2 * x); };
export const smoother = (x) => { x = clamp(x); return x * x * x * (x * (x * 6 - 15) + 10); };
export const easeOut = (x) => 1 - Math.pow(1 - clamp(x), 3);
export const easeIn = (x) => Math.pow(clamp(x), 3);
export const backOut = (x) => { x = clamp(x); const s = 1.5; return 1 + (s + 1) * Math.pow(x - 1, 3) + s * Math.pow(x - 1, 2); };
// A heavy object arriving: quick approach, small settle, no bounce above the target.
export const settle = (x) => { x = clamp(x); return 1 - Math.pow(1 - x, 4) * Math.cos(x * Math.PI * 1.5); };

/** 0 → 1 as t goes from a to b. */
export const ramp = (t, a, b, f = smooth) => f((t - a) / (b - a));
/** Visible from a to b, fading in over fin and out over fout. */
export const win = (t, a, b, fin = 0.6, fout = 0.6) =>
  Math.min(ramp(t, a, a + fin), 1 - ramp(t, b - fout, b));
/** A single bump of height 1 starting at a. */
export const pulse = (t, a, d = 0.8) => { const x = (t - a) / d; return x <= 0 || x >= 1 ? 0 : Math.sin(Math.PI * x); };

export function rng(seed) {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export const v3 = (x = 0, y = 0, z = 0) => new THREE.Vector3(x, y, z);

// ---------- materials ----------
const shared = new Map();
/** A shared, never-animated material. */
export function matS(color, rough = 0.85, metal = 0) {
  const key = `${color}|${rough}|${metal}`;
  if (!shared.has(key)) shared.set(key, new THREE.MeshStandardMaterial({ color, roughness: rough, metalness: metal }));
  return shared.get(key);
}
/** A material of its own, safe to fade or tint. */
export function mat(color, o = {}) {
  return new THREE.MeshStandardMaterial({
    color,
    roughness: o.rough ?? 0.85,
    metalness: o.metal ?? 0,
    emissive: o.emissive ?? 0x000000,
    emissiveIntensity: o.ei ?? 1,
    transparent: o.transparent ?? false,
    opacity: o.opacity ?? 1,
    depthWrite: o.depthWrite ?? true,
    side: o.side ?? THREE.FrontSide,
  });
}
export function glow(color, opacity = 1) {
  return new THREE.MeshBasicMaterial({ color, transparent: opacity < 1, opacity, toneMapped: false });
}

// ---------- meshes ----------
export function mesh(geo, material, { cast = true, receive = true } = {}) {
  const m = new THREE.Mesh(geo, material);
  m.castShadow = cast;
  m.receiveShadow = receive;
  return m;
}
export function rbox(w, h, d, material, r = 0.06) {
  const g = new RoundedBoxGeometry(w, h, d, 2, Math.min(r, w / 2 - 0.001, h / 2 - 0.001, d / 2 - 0.001));
  return mesh(g, typeof material === 'number' ? matS(material) : material);
}
export function place(obj, x, y, z, ry = 0) {
  obj.position.set(x, y, z);
  obj.rotation.y = ry;
  return obj;
}

/** Fade a whole group. Materials must not be shared with objects that stay opaque. */
export function fade(obj, o) {
  obj.visible = o > 0.002;
  if (!obj.visible) return;
  obj.traverse((n) => {
    const m = n.material;
    if (!m) return;
    for (const mm of Array.isArray(m) ? m : [m]) {
      if (mm.userData.baseOpacity === undefined) mm.userData.baseOpacity = mm.opacity;
      const target = mm.userData.baseOpacity * o;
      mm.opacity = target;
      const wantT = target < 0.999 || mm.userData.baseOpacity < 1;
      if (mm.transparent !== wantT) { mm.transparent = wantT; mm.needsUpdate = true; }
    }
  });
}
/** Give a group its own copies of every material so it can be faded independently. */
export function ownMaterials(obj) {
  obj.traverse((n) => { if (n.material) n.material = n.material.clone(); });
  return obj;
}
export function scaleIn(obj, s) {
  obj.visible = s > 0.002;
  obj.scale.setScalar(Math.max(s, 0.0001));
}

// ---------- canvas text (for diegetic signs) ----------
export function canvasTexture(w, h, draw) {
  const cv = document.createElement('canvas');
  cv.width = w; cv.height = h;
  const tex = new THREE.CanvasTexture(cv);
  tex.colorSpace = THREE.SRGBColorSpace;
  tex.anisotropy = 4;
  const ctx = cv.getContext('2d');
  const redraw = (...a) => { ctx.clearRect(0, 0, w, h); draw(ctx, w, h, ...a); tex.needsUpdate = true; };
  redraw();
  return { tex, redraw, ctx };
}

/**
 * A flat sign with text drawn on a canvas. sign.set(text, {fg, bg, glow}) redraws it.
 */
export function sign({ w, h, text, px = 256, font = '700', fg = '#2b2b2b', bg = '#f4ead6', border = null, emissive = false, size = 0.62 }) {
  const cw = Math.round(px * (w / h)), ch = px;
  let state = { text, fg, bg, border };
  const ct = canvasTexture(cw, ch, (ctx, W, H) => {
    ctx.fillStyle = state.bg; ctx.fillRect(0, 0, W, H);
    if (state.border) { ctx.strokeStyle = state.border; ctx.lineWidth = H * 0.06; ctx.strokeRect(H * 0.05, H * 0.05, W - H * 0.1, H - H * 0.1); }
    ctx.fillStyle = state.fg;
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    const lines = String(state.text).split('\n');
    let fs = H * size / lines.length;
    ctx.font = `${font} ${fs}px Inter, system-ui, sans-serif`;
    const widest = Math.max(...lines.map((l) => ctx.measureText(l).width));
    if (widest > W * 0.9) { fs *= (W * 0.9) / widest; ctx.font = `${font} ${fs}px Inter, system-ui, sans-serif`; }
    lines.forEach((l, i) => ctx.fillText(l, W / 2, H / 2 + (i - (lines.length - 1) / 2) * fs * 1.15));
  });
  const material = emissive
    ? new THREE.MeshBasicMaterial({ map: ct.tex, transparent: true, toneMapped: false })
    : new THREE.MeshStandardMaterial({ map: ct.tex, roughness: 0.9, transparent: true });
  const m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), material);
  m.userData.set = (t, o = {}) => {
    const next = { ...state, text: t ?? state.text, ...o };
    if (next.text === state.text && next.fg === state.fg && next.bg === state.bg && next.border === state.border) return;
    state = next;
    ct.redraw();
  };
  return m;
}

// ---------- people ----------
const bodyGeo = new THREE.CapsuleGeometry(0.24, 0.36, 4, 12);
const headGeo = new THREE.SphereGeometry(0.19, 18, 14);
export const FIG = { bodyGeo, headGeo, bodyY: 0.42, headY: 1.0 };

/**
 * A small figure: capsule body, round head. Optional apron and notebook mark Jia.
 */
export function figure({ shirt = 0x5b7fa6, skin = 0xf1d0b5, apron = false, notebook = false, hair = null } = {}) {
  const g = new THREE.Group();
  const body = mesh(bodyGeo, mat(shirt, { rough: 0.8 }));
  body.position.y = FIG.bodyY;
  const headPivot = new THREE.Group();
  headPivot.position.y = FIG.headY;
  const head = mesh(headGeo, mat(skin, { rough: 0.7 }));
  headPivot.add(head);
  if (hair !== null) {
    const h = mesh(new THREE.SphereGeometry(0.195, 18, 10, 0, Math.PI * 2, 0, Math.PI * 0.55), mat(hair, { rough: 0.9 }));
    h.rotation.x = -0.25;
    h.position.z = -0.01;
    headPivot.add(h);
  }
  g.add(body, headPivot);
  if (apron) {
    const a = rbox(0.4, 0.5, 0.05, mat(PAL.apron, { rough: 0.9 }), 0.02);
    a.position.set(0, 0.42, 0.22);
    const strap = rbox(0.06, 0.2, 0.03, mat(PAL.apron), 0.01);
    strap.position.set(0, 0.74, 0.2);
    g.add(a, strap);
  }
  let book = null;
  if (notebook) {
    book = rbox(0.2, 0.26, 0.04, mat(0xf7f3ea), 0.01);
    book.position.set(0.3, 0.5, 0.12);
    book.rotation.set(-0.5, 0, 0.15);
    g.add(book);
  }
  g.userData = { body, headPivot, book, base: 0 };
  return g;
}

/** Walking bob and head turn. phase advances with distance walked. */
export function pose(fig, { walking = 0, phase = 0, look = 0, nod = 0 } = {}) {
  const u = fig.userData;
  const bob = Math.abs(Math.sin(phase)) * 0.06 * walking;
  u.body.position.y = FIG.bodyY + bob;
  u.body.rotation.z = Math.sin(phase) * 0.06 * walking;
  u.headPivot.position.y = FIG.headY + bob;
  u.headPivot.rotation.y = look;
  u.headPivot.rotation.x = nod;
}

/** A seated figure (shorter body, no bob). */
export function seated(opts) {
  const f = figure(opts);
  f.userData.body.scale.y = 0.8;
  f.userData.body.position.y = FIG.bodyY * 0.8 + 0.28;
  f.userData.headPivot.position.y = FIG.headY * 0.8 + 0.28;
  f.userData.seated = true;
  return f;
}

export function pick(r, list) { return list[Math.floor(r() * list.length)]; }

// ---------- furniture and small props ----------
const bookColors = [0xb85c4a, 0x4f6f9a, 0xd9b45a, 0x6f9a7a, 0x8a6fae, 0xe0ddd2, 0x9b4f3c, 0x3f5a6e, 0xc9a27a];

/** A shelf unit filled with books. messy = some books lying flat or missing. */
export function shelf(w, h, d, { seed = 1, rows = 4, messy = false, frame = PAL.woodDark } = {}) {
  const g = new THREE.Group();
  const r = rng(seed);
  const fm = matS(frame, 0.8);
  const t = 0.05;
  g.add(place(rbox(t, h, d, fm, 0.01), -w / 2, h / 2, 0));
  g.add(place(rbox(t, h, d, fm, 0.01), w / 2, h / 2, 0));
  g.add(place(rbox(w, t, d, fm, 0.01), 0, h - t / 2, 0));
  g.add(place(rbox(w, 0.02, d, fm, 0.005), 0, h / 2, -d / 2 + 0.01));
  const rowH = (h - t) / rows;
  // All the books on one shelf are a single instanced mesh.
  const books = [];
  const o = new THREE.Object3D();
  for (let i = 0; i < rows; i++) {
    const y = i * rowH;
    g.add(place(rbox(w, t, d, fm, 0.01), 0, y + t / 2, 0));
    let x = -w / 2 + t;
    while (x < w / 2 - t - 0.06) {
      const bw = 0.05 + r() * 0.05;
      const bh = rowH * (0.55 + r() * 0.3);
      if (messy && r() < 0.25) { x += bw * 1.5; continue; }
      o.position.set(x + bw / 2, y + t + bh / 2, 0.01);
      o.rotation.set(0, 0, 0);
      if (messy && r() < 0.2) { o.rotation.z = 0.35; o.position.x += 0.03; }
      o.scale.set(bw, bh, d * 0.8);
      o.updateMatrix();
      books.push([o.matrix.clone(), pick(r, bookColors)]);
      x += bw + 0.008;
    }
  }
  const inst = new THREE.InstancedMesh(unitBox, bookMat, books.length);
  books.forEach(([m, col], k) => { inst.setMatrixAt(k, m); inst.setColorAt(k, new THREE.Color(col)); });
  inst.receiveShadow = true;
  g.add(inst);
  return g;
}
const unitBox = new THREE.BoxGeometry(1, 1, 1);
const bookMat = new THREE.MeshStandardMaterial({ roughness: 0.9 });

export function table(w = 1.1, d = 0.75, h = 0.62, top = PAL.wood) {
  const g = new THREE.Group();
  g.add(place(rbox(w, 0.06, d, matS(top, 0.7), 0.02), 0, h, 0));
  const lm = matS(PAL.woodDark);
  for (const [x, z] of [[-1, -1], [1, -1], [-1, 1], [1, 1]]) {
    g.add(place(mesh(new THREE.CylinderGeometry(0.03, 0.03, h, 6), lm), x * (w / 2 - 0.08), h / 2, z * (d / 2 - 0.08)));
  }
  return g;
}

export function roundTable(r = 0.5, h = 0.62) {
  const g = new THREE.Group();
  g.add(place(mesh(new THREE.CylinderGeometry(r, r, 0.06, 24), matS(PAL.wood, 0.7)), 0, h, 0));
  g.add(place(mesh(new THREE.CylinderGeometry(0.05, 0.05, h, 8), matS(PAL.woodDark)), 0, h / 2, 0));
  g.add(place(mesh(new THREE.CylinderGeometry(0.25, 0.25, 0.04, 16), matS(PAL.woodDark)), 0, 0.02, 0));
  return g;
}

export function stool() {
  const g = new THREE.Group();
  g.add(place(mesh(new THREE.CylinderGeometry(0.18, 0.18, 0.05, 14), matS(PAL.woodDark)), 0, 0.42, 0));
  g.add(place(mesh(new THREE.CylinderGeometry(0.04, 0.05, 0.42, 6), matS(PAL.woodDark)), 0, 0.21, 0));
  return g;
}

export function sofa(color = 0x8a6f5a) {
  const g = new THREE.Group();
  const m = matS(color, 0.95);
  g.add(place(rbox(1.6, 0.35, 0.7, m, 0.1), 0, 0.25, 0));
  g.add(place(rbox(1.6, 0.5, 0.2, m, 0.08), 0, 0.55, -0.27));
  g.add(place(rbox(0.2, 0.45, 0.7, m, 0.08), -0.8, 0.4, 0));
  g.add(place(rbox(0.2, 0.45, 0.7, m, 0.08), 0.8, 0.4, 0));
  return g;
}

export function counter(w = 2.2, color = 0x9c7452) {
  const g = new THREE.Group();
  g.add(place(rbox(w, 0.9, 0.7, matS(color, 0.8), 0.04), 0, 0.45, 0));
  g.add(place(rbox(w + 0.1, 0.06, 0.8, matS(0xd8c7a8, 0.6), 0.02), 0, 0.93, 0));
  return g;
}

export function lampPost(h = 2.6) {
  const g = new THREE.Group();
  g.add(place(mesh(new THREE.CylinderGeometry(0.05, 0.07, h, 8), matS(0x3b4250, 0.6, 0.3)), 0, h / 2, 0));
  const bulb = new THREE.Mesh(new THREE.SphereGeometry(0.16, 14, 10), new THREE.MeshStandardMaterial({ color: 0xfff1d6, emissive: 0xffd08a, emissiveIntensity: 0 }));
  bulb.position.y = h + 0.05;
  g.add(bulb);
  g.userData.bulb = bulb;
  return g;
}

export function tree(r, s = 1) {
  const g = new THREE.Group();
  const trunkH = 0.9 * s;
  g.add(place(mesh(new THREE.CylinderGeometry(0.08 * s, 0.12 * s, trunkH, 6), matS(0x6b4f3a)), 0, trunkH / 2, 0));
  const green = pick(r, [0x6f9a62, 0x5d8a58, 0x7fa86a, 0x8bb070]);
  if (r() < 0.45) {
    g.add(place(mesh(new THREE.ConeGeometry(0.75 * s, 1.9 * s, 8), matS(green, 0.9)), 0, trunkH + 0.8 * s, 0));
  } else {
    g.add(place(mesh(new THREE.IcosahedronGeometry(0.85 * s, 1), matS(green, 0.9)), 0, trunkH + 0.6 * s, 0));
    g.add(place(mesh(new THREE.IcosahedronGeometry(0.55 * s, 1), matS(green, 0.9)), 0.35 * s, trunkH + 1.15 * s, 0.1 * s));
  }
  return g;
}

export function hedge(len, h = 0.8, d = 0.6) {
  return rbox(len, h, d, mat(0x5f8a55, { rough: 0.95 }), 0.25);
}

// ---------- lines and threads ----------
/** An arc between two points, lifted by `lift` at the middle. */
export function arc(a, b, lift) {
  const mid = a.clone().add(b).multiplyScalar(0.5);
  mid.y += lift;
  return new THREE.QuadraticBezierCurve3(a.clone(), mid, b.clone());
}

/**
 * A tube that can be drawn on progressively: tube.userData.grow(p) shows the first fraction p.
 */
export function growTube(curve, radius, material, segs = 80, radial = 8) {
  const geo = new THREE.TubeGeometry(curve, segs, radius, radial, false);
  const m = mesh(geo, material, { cast: false, receive: false });
  const perSeg = radial * 6;
  m.userData.grow = (p) => {
    p = clamp(p);
    m.visible = p > 0.001;
    geo.setDrawRange(0, Math.ceil(segs * p) * perSeg);
  };
  m.userData.grow(0);
  return m;
}

/** A soft round shadow / glow disc lying on the ground. */
export function disc(radius, color, opacity = 0.5) {
  const m = new THREE.Mesh(new THREE.CircleGeometry(radius, 40), new THREE.MeshBasicMaterial({ color, transparent: true, opacity, depthWrite: false, toneMapped: false }));
  m.rotation.x = -Math.PI / 2;
  return m;
}

export function ringMesh(r, tube, color, opacity = 1) {
  const m = new THREE.Mesh(new THREE.TorusGeometry(r, tube, 8, 64), new THREE.MeshBasicMaterial({ color, transparent: true, opacity, depthWrite: false, toneMapped: false }));
  m.rotation.x = -Math.PI / 2;
  return m;
}

/** Sample a polyline (array of [x, z]) by arc length, with rounded corners. */
export function polyPath(points, y = 0) {
  const pts = points.map(([x, z]) => new THREE.Vector3(x, y, z));
  const curve = new THREE.CatmullRomCurve3(pts, false, 'catmullrom', 0.05);
  curve.arcLengthDivisions = 400;
  return curve;
}
