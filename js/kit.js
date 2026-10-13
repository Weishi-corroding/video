// Shared toolkit: palette, materials, easing, deterministic animation helpers,
// and the miniature props reused across chapters (desks, figures, AI panel...).
import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

export { THREE };

export const COLORS = {
  bg: 0x082427,
  ground: 0x0b2f33,
  plinth: 0x114246,
  plinthEdge: 0x1a5559,
  ivory: 0xf1e7d3,
  ivory2: 0xe3d6bc,
  ivory3: 0xc9b998,
  ink: 0x24474a,
  amber: 0xf2a33a,
  mint: 0x7fe3c3,
};

// ---------- easing & time helpers (all pure functions of time) ----------
export const clamp01 = (x) => (x < 0 ? 0 : x > 1 ? 1 : x);
export const lerp = (a, b, k) => a + (b - a) * k;
export const ease = {
  linear: (x) => x,
  inOut: (x) => (x < 0.5 ? 4 * x * x * x : 1 - Math.pow(-2 * x + 2, 3) / 2),
  out: (x) => 1 - Math.pow(1 - x, 3),
  in: (x) => x * x * x,
  back: (x) => {
    const c1 = 1.4, c3 = c1 + 1;
    return 1 + c3 * Math.pow(x - 1, 3) + c1 * Math.pow(x - 1, 2);
  },
};
/** 0→1 between a and b, eased. */
export const ramp = (t, a, b, fn = ease.inOut) => fn(clamp01((t - a) / (b - a)));
/** 0→1→0 window with fades of length f at both ends. */
export const win = (t, a, b, f = 0.5) => Math.min(ramp(t, a, a + f), 1 - ramp(t, b - f, b));
/** Deterministic pseudo-random in [0,1) from an integer. */
export const hash = (n) => {
  const s = Math.sin(n * 127.1 + 311.7) * 43758.5453;
  return s - Math.floor(s);
};

// ---------- geometry & materials ----------
const geoCache = new Map();
export function rboxGeo(w, h, d, r = 0.035) {
  const key = `rb:${w}:${h}:${d}:${r}`;
  if (!geoCache.has(key)) {
    const rr = Math.min(r, w / 2 - 1e-3, h / 2 - 1e-3, d / 2 - 1e-3);
    geoCache.set(key, new RoundedBoxGeometry(w, h, d, 2, Math.max(rr, 0.001)));
  }
  return geoCache.get(key);
}

export function std(color, opts = {}) {
  return new THREE.MeshStandardMaterial({ color, roughness: 0.82, metalness: 0, ...opts });
}
export function glow(color, intensity = 1.6, opts = {}) {
  return new THREE.MeshStandardMaterial({ color, emissive: color, emissiveIntensity: intensity, roughness: 0.45, ...opts });
}

export const MAT = {
  ivory: std(COLORS.ivory),
  ivory2: std(COLORS.ivory2),
  ivory3: std(COLORS.ivory3),
  paper: std(0xfaf3e4, { roughness: 0.9 }),
  ink: std(COLORS.ink, { roughness: 0.7 }),
  plinth: std(COLORS.plinth, { roughness: 0.95 }),
  plinthEdge: std(COLORS.plinthEdge, { roughness: 0.9 }),
  amber: std(COLORS.amber, { roughness: 0.55 }),
  mint: std(COLORS.mint, { roughness: 0.55 }),
  amberGlow: glow(COLORS.amber, 1.6),
  mintGlow: glow(COLORS.mint, 1.5),
  amberSoft: glow(COLORS.amber, 0.55),
  mintSoft: glow(COLORS.mint, 0.5),
  glass: new THREE.MeshPhysicalMaterial({
    color: 0xbfeee2, roughness: 0.15, metalness: 0, transparent: true, opacity: 0.22,
    emissive: 0x2a6f66, emissiveIntensity: 0.35, side: THREE.DoubleSide, depthWrite: false,
  }),
};

export function mesh(geo, mat, { cast = true, receive = true } = {}) {
  const m = new THREE.Mesh(geo, mat);
  m.castShadow = cast;
  m.receiveShadow = receive;
  return m;
}
export const box = (w, h, d, mat = MAT.ivory, r) => mesh(rboxGeo(w, h, d, r), mat);
export const cyl = (rt, rb, h, mat = MAT.ivory, seg = 32) => mesh(new THREE.CylinderGeometry(rt, rb, h, seg), mat);
export const sphere = (r, mat = MAT.ivory, seg = 24) => mesh(new THREE.SphereGeometry(r, seg, Math.round(seg * 0.75)), mat);

export function at(obj, x = 0, y = 0, z = 0) {
  obj.position.set(x, y, z);
  return obj;
}
export function group(...children) {
  const g = new THREE.Group();
  children.forEach((c) => c && g.add(c));
  return g;
}

// ---------- deterministic visibility helpers ----------
function base(obj) {
  if (!obj.userData.s0) obj.userData.s0 = obj.scale.clone();
  if (!obj.userData.p0) obj.userData.p0 = obj.position.clone();
  return obj.userData;
}
/** Scale-in (with a little overshoot) driven by k in [0,1]. */
export function pop(obj, k, fn = ease.back) {
  const u = base(obj);
  const s = k <= 0 ? 0 : k >= 1 ? 1 : fn(k);
  obj.visible = k > 0.001;
  obj.scale.copy(u.s0).multiplyScalar(Math.max(s, 1e-4));
}
/** Rise from below its resting position, driven by k in [0,1]. */
export function rise(obj, k, depth = 0.8) {
  const u = base(obj);
  obj.visible = k > 0.001;
  obj.position.y = u.p0.y - depth * (1 - ease.out(clamp01(k)));
}
/** Offset relative to resting position. */
export function offset(obj, x = 0, y = 0, z = 0) {
  const u = base(obj);
  obj.position.set(u.p0.x + x, u.p0.y + y, u.p0.z + z);
}
/** Opacity fade; clones materials on first use so fades stay per-object. */
export function fade(obj, o) {
  obj.visible = o > 0.003;
  if (!obj.visible) return;
  obj.traverse((m) => {
    if (!m.material) return;
    if (!m.userData.fadeMat) {
      m.material = m.material.clone();
      m.material.transparent = true;
      m.userData.fadeMat = true;
      m.userData.o0 = m.material.opacity;
    }
    m.material.opacity = m.userData.o0 * o;
    m.material.depthWrite = o > 0.6;
  });
}
/** Set emissive intensity on every mesh under obj (materials cloned once). */
export function setGlow(obj, intensity, color) {
  obj.traverse((m) => {
    if (!m.material || !m.material.emissive) return;
    if (!m.userData.glowMat) {
      m.material = m.material.clone();
      m.userData.glowMat = true;
    }
    if (color !== undefined) m.material.emissive.set(color);
    m.material.emissiveIntensity = intensity;
  });
}

// ---------- paths ----------
/** A tube along a smooth curve that can be drawn progressively. */
export function tube(points, r, mat, { segs = 140, radial = 8, closed = false } = {}) {
  const curve = new THREE.CatmullRomCurve3(points.map((p) => new THREE.Vector3(...p)), closed, 'catmullrom', 0.5);
  const geo = new THREE.TubeGeometry(curve, segs, r, radial, closed);
  const m = mesh(geo, mat, { cast: false, receive: false });
  m.userData.curve = curve;
  m.userData.progress = (p) => {
    const n = Math.round(clamp01(p) * segs);
    geo.setDrawRange(0, n * radial * 6);
    m.visible = n > 0;
  };
  return m;
}

/** Dashed line along points, progressively drawable. */
export function dashed(points, color, { dash = 0.18, gap = 0.12, opacity = 0.9, segs = 120 } = {}) {
  const curve = new THREE.CatmullRomCurve3(points.map((p) => new THREE.Vector3(...p)));
  const geo = new THREE.BufferGeometry().setFromPoints(curve.getPoints(segs));
  const mat = new THREE.LineDashedMaterial({ color, dashSize: dash, gapSize: gap, transparent: true, opacity });
  const line = new THREE.Line(geo, mat);
  line.computeLineDistances();
  line.userData.curve = curve;
  line.userData.progress = (p) => {
    const n = Math.round(clamp01(p) * (segs + 1));
    geo.setDrawRange(0, n);
    line.visible = n > 1;
  };
  return line;
}

/** Dashed path built from short 3D segments (visible at any resolution), progressively drawable. */
export function dashedTube(points, color, { dash = 0.22, gap = 0.14, r = 0.028, emissive = 0.9, opacity = 1 } = {}) {
  const curve = new THREE.CatmullRomCurve3(points.map((p) => new THREE.Vector3(...p)));
  const len = curve.getLength();
  const n = Math.max(1, Math.floor(len / (dash + gap)));
  const mat = new THREE.MeshStandardMaterial({ color, emissive: color, emissiveIntensity: emissive, roughness: 0.5, transparent: opacity < 1, opacity });
  const im = new THREE.InstancedMesh(new THREE.CylinderGeometry(r, r, 1, 8), mat, n);
  const m4 = new THREE.Matrix4(), q = new THREE.Quaternion(), up = new THREE.Vector3(0, 1, 0);
  const a = new THREE.Vector3(), b = new THREE.Vector3(), mid = new THREE.Vector3(), dir = new THREE.Vector3(), sc = new THREE.Vector3();
  for (let i = 0; i < n; i++) {
    const u0 = (i * (dash + gap)) / len, u1 = Math.min(1, u0 + dash / len);
    curve.getPointAt(u0, a);
    curve.getPointAt(u1, b);
    mid.addVectors(a, b).multiplyScalar(0.5);
    dir.subVectors(b, a);
    const l = dir.length();
    q.setFromUnitVectors(up, dir.normalize());
    m4.compose(mid, q, sc.set(1, l, 1));
    im.setMatrixAt(i, m4);
  }
  im.castShadow = false;
  im.userData.curve = curve;
  im.userData.progress = (p) => {
    im.count = Math.round(clamp01(p) * n);
    im.visible = im.count > 0;
  };
  im.userData.progress(0);
  return im;
}

// ---------- props ----------
export function desk() {
  const g = new THREE.Group();
  g.add(at(box(1.3, 0.07, 0.72, MAT.ivory, 0.025), 0, 0.7, 0));
  for (const [x, z] of [[-0.58, -0.3], [0.58, -0.3], [-0.58, 0.3], [0.58, 0.3]]) {
    g.add(at(box(0.06, 0.68, 0.06, MAT.ivory2, 0.01), x, 0.34, z));
  }
  g.userData.top = 0.735;
  return g;
}

export function chair() {
  const g = new THREE.Group();
  g.add(at(box(0.46, 0.06, 0.44, MAT.ivory2, 0.02), 0, 0.44, 0));
  g.add(at(box(0.46, 0.5, 0.06, MAT.ivory2, 0.02), 0, 0.72, -0.2));
  for (const [x, z] of [[-0.19, -0.18], [0.19, -0.18], [-0.19, 0.18], [0.19, 0.18]]) {
    g.add(at(box(0.05, 0.43, 0.05, MAT.ivory3, 0.01), x, 0.215, z));
  }
  return g;
}

/**
 * Abstract miniature person. seated=true puts the hips at chair height.
 * who='A' adds a scarf and who='B' a knit cap: neutral silhouette markers, so
 * the two students stay recognisable without colour-coding either of them.
 */
export function figure({ seated = true, scale = 1, mat = MAT.ivory, who } = {}) {
  const g = new THREE.Group();
  const body = mesh(new THREE.CapsuleGeometry(0.17, seated ? 0.3 : 0.62, 6, 16), mat);
  const bodyY = seated ? 0.47 + 0.32 : 0.17 + 0.31;
  body.position.y = bodyY;
  const torsoTop = bodyY + (seated ? 0.15 : 0.31) + 0.17;
  const headPivot = new THREE.Group();
  headPivot.position.y = torsoTop + 0.03;
  const head = at(sphere(0.15, mat), 0, 0.15, 0);
  headPivot.add(head);
  g.add(body, headPivot);
  if (seated) {
    // thighs reaching forward to the desk
    g.add(at(box(0.3, 0.12, 0.4, mat, 0.05), 0, 0.53, 0.14));
  }
  if (who === 'A') {
    const scarf = at(mesh(new THREE.TorusGeometry(0.135, 0.05, 10, 28), MAT.ivory3), 0, torsoTop - 0.03, 0);
    scarf.rotation.x = Math.PI / 2;
    const tail = at(box(0.09, 0.24, 0.035, MAT.ivory3, 0.015), 0.07, torsoTop - 0.17, 0.19);
    tail.rotation.z = 0.12;
    g.add(scarf, tail);
  } else if (who === 'B') {
    const cap = at(mesh(new THREE.SphereGeometry(0.158, 24, 10, 0, Math.PI * 2, 0, Math.PI / 2), MAT.ivory3), 0, 0.16, 0);
    const band = at(mesh(new THREE.TorusGeometry(0.155, 0.028, 8, 32), MAT.ivory3), 0, 0.17, 0);
    band.rotation.x = Math.PI / 2;
    const pom = sphere(0.045, MAT.ivory3, 12);
    pom.position.y = 0.33;
    headPivot.add(cap, band, pom);
  }
  g.scale.setScalar(scale);
  g.userData.head = headPivot;
  g.userData.body = body;
  g.userData.headTop = (torsoTop + 0.33) * scale;
  return g;
}

export function lamp() {
  const g = new THREE.Group();
  g.add(at(cyl(0.09, 0.1, 0.03, MAT.ivory2), 0, 0.015, 0));
  const arm = at(cyl(0.012, 0.012, 0.42, MAT.ivory3, 8), 0, 0.22, 0);
  g.add(arm);
  const shade = at(cyl(0.05, 0.12, 0.1, MAT.ivory, 24), 0.05, 0.44, 0);
  shade.rotation.z = -0.5;
  g.add(shade);
  const bulb = at(sphere(0.035, glow(0xffd9a0, 2.2)), 0.07, 0.4, 0);
  g.add(bulb);
  return g;
}

export function books(n = 3, seed = 1) {
  const g = new THREE.Group();
  let y = 0;
  for (let i = 0; i < n; i++) {
    const h = 0.045 + hash(seed * 10 + i) * 0.03;
    const b = box(0.3 - i * 0.02, h, 0.22, i % 2 ? MAT.ivory2 : MAT.ivory3, 0.01);
    b.position.set(0, y + h / 2, 0);
    b.rotation.y = (hash(seed * 20 + i) - 0.5) * 0.5;
    g.add(b);
    y += h;
  }
  return g;
}

export function laptop() {
  const g = new THREE.Group();
  g.add(at(box(0.42, 0.02, 0.3, MAT.ivory3, 0.008), 0, 0.01, 0));
  const lid = new THREE.Group();
  lid.position.set(0, 0.02, 0.15);
  const screen = at(box(0.42, 0.28, 0.015, MAT.ivory3, 0.008), 0, 0.14, 0);
  const face = at(mesh(new THREE.PlaneGeometry(0.37, 0.23), glow(COLORS.mint, 0.35)), 0, 0.14, -0.009);
  face.rotation.y = Math.PI;
  lid.add(screen, face);
  lid.rotation.x = 0.28;
  g.add(lid);
  g.userData.face = face;
  return g;
}

/** A sheet of paper with text lines; returns {group, lines[]} for animating. */
export function sheet({ w = 0.34, d = 0.44, lines = 5, mat = MAT.paper, lineMat = MAT.ink, seed = 3 } = {}) {
  const g = new THREE.Group();
  g.add(at(box(w, 0.012, d, mat, 0.004), 0, 0.006, 0));
  const ls = [];
  for (let i = 0; i < lines; i++) {
    const lw = (w - 0.08) * (0.55 + hash(seed + i) * 0.45);
    const l = at(box(lw, 0.004, 0.022, lineMat, 0.002), -(w - 0.08) / 2 + lw / 2, 0.014, -d / 2 + 0.07 + i * ((d - 0.12) / Math.max(1, lines - 1)));
    ls.push(l);
    g.add(l);
  }
  g.userData.lines = ls;
  return g;
}

/** A polished answer card: thicker, with an accent stripe. */
export function card({ w = 0.5, h = 0.66, accent = MAT.amberGlow, lines = 6 } = {}) {
  const g = new THREE.Group();
  g.add(at(box(w, 0.03, h, MAT.paper, 0.012), 0, 0, 0));
  g.add(at(box(w * 0.9, 0.012, 0.05, accent, 0.004), 0, 0.02, -h / 2 + 0.06));
  for (let i = 0; i < lines; i++) {
    const lw = w * (0.62 + 0.22 * ((i * 7) % 3) / 2);
    g.add(at(box(lw, 0.008, 0.022, MAT.ivory3, 0.002), -w * 0.4 + lw / 2, 0.02, -h / 2 + 0.16 + i * ((h - 0.24) / (lines - 1))));
  }
  return g;
}

export function plinth(r = 7.5, h = 0.5) {
  const g = new THREE.Group();
  const top = at(cyl(r, r, h, MAT.plinth, 96), 0, -h / 2, 0);
  top.castShadow = false;
  const rim = at(mesh(new THREE.TorusGeometry(r, 0.03, 8, 160), MAT.mintSoft, { cast: false }), 0, 0.0, 0);
  rim.rotation.x = Math.PI / 2;
  fade(rim, 0.28);
  g.add(top, rim);
  return g;
}

/** The floating AI interface shared by both students. */
export function aiPanel({ w = 1.9, h = 1.15 } = {}) {
  const g = new THREE.Group();
  const panel = mesh(rboxGeo(w, h, 0.05, 0.08), MAT.glass, { cast: false });
  g.add(panel);
  const frameMat = glow(COLORS.amber, 1.1);
  const t = 0.035;
  for (const [x, y, fw, fh] of [[0, h / 2, w, t], [0, -h / 2, w, t], [-w / 2, 0, t, h], [w / 2, 0, t, h]]) {
    g.add(at(mesh(rboxGeo(fw, fh, 0.06, 0.012), frameMat, { cast: false }), x, y, 0));
  }
  const core = new THREE.Group();
  const ring = mesh(new THREE.TorusGeometry(0.22, 0.025, 12, 64), glow(COLORS.mint, 1.4), { cast: false });
  const orb = sphere(0.09, glow(COLORS.amber, 2.0));
  core.add(ring, orb);
  core.position.z = 0.05;
  g.add(core);
  // a few "text" bars on the panel
  for (let i = 0; i < 3; i++) {
    g.add(at(mesh(rboxGeo(0.5 - i * 0.1, 0.03, 0.01, 0.005), glow(COLORS.ivory, 0.6), { cast: false }), 0, -0.32 - i * 0.07 + 0.0, 0.04));
  }
  g.userData.ring = ring;
  g.userData.core = core;
  g.userData.frameMat = frameMat;
  g.userData.animate = (t) => {
    ring.rotation.x = t * 0.9;
    ring.rotation.y = t * 0.6;
    core.position.y = 0.08 + Math.sin(t * 1.3) * 0.02;
  };
  return g;
}

/** Small glowing data packet. */
export function packet(mat = MAT.amberGlow, s = 0.12) {
  return box(s, s * 0.6, s * 1.3, mat, 0.02);
}

/** A seated student at a desk on a low platform. Faces local +z. who: 'A' | 'B'. */
export function studySpace({ seed = 1, platform = [3.0, 0.14, 2.4], who } = {}) {
  const g = new THREE.Group();
  const [pw, ph, pd] = platform;
  const base0 = at(box(pw, ph, pd, MAT.ivory2, 0.05), 0, ph / 2, 0);
  g.add(base0);
  const y0 = ph;
  const d = at(desk(), 0, y0, 0.35);
  const c = at(chair(), 0, y0, -0.25);
  const f = at(figure({ seated: true, who }), 0, y0, -0.28);
  const top = y0 + d.userData.top;
  const lp = at(laptop(), 0.1, top, 0.5); // lid at the far edge, screen facing the student
  const lmp = at(lamp(), -0.5, top, 0.55);
  const bk = at(books(3, seed), 0.48, top, 0.5);
  const paperSheet = at(sheet({ seed: seed * 7 }), -0.18, top, 0.24);
  paperSheet.rotation.y = 0.12;
  g.add(d, c, f, lp, lmp, bk, paperSheet);
  g.userData = { figure: f, laptop: lp, paper: paperSheet, deskTop: top, base: base0 };
  return g;
}

/** Upright arrow made of a shaft and cone; points along +y before rotation. */
export function arrow(len = 1, mat = MAT.amberGlow, r = 0.035) {
  const g = new THREE.Group();
  g.add(at(cyl(r, r, len * 0.75, mat, 12), 0, len * 0.375, 0));
  g.add(at(mesh(new THREE.ConeGeometry(r * 2.6, len * 0.25, 16), mat), 0, len * 0.875, 0));
  return g;
}

/** Simple camera keyframe track: [{t, p:[x,y,z], l:[x,y,z]}], eased between keys. */
export function camTrack(keys) {
  const P = keys.map((k) => new THREE.Vector3(...k.p));
  const L = keys.map((k) => new THREE.Vector3(...k.l));
  return (t, outP, outL) => {
    if (t <= keys[0].t) { outP.copy(P[0]); outL.copy(L[0]); return; }
    for (let i = 0; i < keys.length - 1; i++) {
      if (t < keys[i + 1].t) {
        const k = ease.inOut((t - keys[i].t) / (keys[i + 1].t - keys[i].t));
        outP.lerpVectors(P[i], P[i + 1], k);
        outL.lerpVectors(L[i], L[i + 1], k);
        return;
      }
    }
    outP.copy(P[P.length - 1]);
    outL.copy(L[L.length - 1]);
  };
}

/** Walk-in-place bob for a standing figure. */
export function walkBob(fig, t, amount = 1) {
  const b = fig.userData.body;
  if (!b.userData.y0) b.userData.y0 = b.position.y;
  b.position.y = b.userData.y0 + Math.abs(Math.sin(t * 7)) * 0.03 * amount;
  fig.rotation.z = Math.sin(t * 7) * 0.04 * amount;
}
