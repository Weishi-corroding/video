// Chapter 6 · Back, but Higher
// The shop's history as a helix: swap shelf → shop → swap-and-study room. From above a circle, from the side a climb.
import * as THREE from 'three';
import {
  win, ramp, backOut, lerp, rng, pick, PAL, seated, figure, place, matS, rbox, sign, sofa, shelf, counter, table,
  scaleIn, fade, glow,
} from '../kit.js';

export function build({ W, L, c, chapters }) {
  const ch = chapters[5];
  const T = (id, dt = 0) => c[id].s + dt;

  W.addLight([{ t: T('6a', 1.0), p: 'night', d: 3 }]);
  W.addCrowd([{ t: T('6a', 1.0), v: 0, d: 2 }]);

  // Helix geometry: one full turn from stage 1 to stage 3, continuing faintly beyond.
  const C = new THREE.Vector3(17, 0, -25);
  const R = 5.2, Y0 = 6.5, Y1 = 18.5, TH = 0.6;
  const at = (s, out = new THREE.Vector3()) => out.set(C.x + Math.sin(TH + s * Math.PI * 2) * R, Y0 + (Y1 - Y0) * s, C.z + Math.cos(TH + s * Math.PI * 2) * R);
  class Helix extends THREE.Curve {
    constructor(s0, s1) { super(); this.s0 = s0; this.s1 = s1; }
    getPoint(u, o = new THREE.Vector3()) { return at(lerp(this.s0, this.s1, u), o); }
  }
  const tubeMat = new THREE.MeshBasicMaterial({ color: 0xffe3a0, transparent: true, opacity: 0.95, toneMapped: false });
  const tubeGeo = new THREE.TubeGeometry(new Helix(0, 1), 240, 0.13, 10, false);
  const tube = new THREE.Mesh(tubeGeo, tubeMat);
  const ghostTube = new THREE.Mesh(new THREE.TubeGeometry(new Helix(0, 1.22), 260, 0.06, 6, false), new THREE.MeshBasicMaterial({ color: 0xbfc8e8, transparent: true, opacity: 0.25, toneMapped: false, depthWrite: false }));
  const beyond = new THREE.Mesh(new THREE.TubeGeometry(new Helix(1, 1.22), 60, 0.1, 8, false), new THREE.MeshBasicMaterial({ color: 0xffe3a0, transparent: true, opacity: 0.4, toneMapped: false, depthWrite: false }));
  const helix = new THREE.Group();
  helix.add(tube, ghostTube, beyond);
  W.scene.add(helix);
  const perSeg = 10 * 6;
  const growTo = (s) => tubeGeo.setDrawRange(0, Math.ceil(240 * Math.min(1, Math.max(0, s))) * perSeg);

  // A column of dots from stage 1 to stage 3: same direction, different height.
  const column = new THREE.Group();
  for (let i = 1; i < 16; i++) {
    const d = new THREE.Mesh(new THREE.SphereGeometry(0.09, 10, 8), glow(0xffffff, 0.9));
    d.position.copy(at(0)).setY(lerp(Y0, Y1, i / 16));
    column.add(d);
  }
  W.scene.add(column);

  // ---- three stages ----
  const r = rng(71);
  const person = (s = false, o = {}) => (s ? seated : figure)({ shirt: pick(r, PAL.shirts), skin: pick(r, PAL.skin), hair: r() < 0.7 ? pick(r, [0x2a2220, 0x4a3426, 0x1d1d22]) : null, ...o });
  const stage = (s, build) => {
    const g = new THREE.Group();
    g.add(place(new THREE.Mesh(new THREE.CylinderGeometry(2.4, 2.3, 0.3, 48), matS(0xefe6d4, 0.8)), 0, -0.15, 0));
    const inner = new THREE.Group();
    build(inner);
    g.add(inner);
    at(s, g.position);
    g.rotation.y = TH;   // every stage faces the same way, toward the viewer's side
    W.scene.add(g);
    return g;
  };
  // Stage 1: a lounge, a messy swap shelf, books on the floor.
  const lost = [];
  const s1 = stage(0, (g) => {
    g.add(place(new THREE.Mesh(new THREE.CylinderGeometry(1.5, 1.5, 0.02, 40), matS(0xb5694f, 0.95)), 0, 0.01, 0.2));
    g.add(place(sofa(0x7c8fa6), -0.5, 0, 0.3, 0));
    g.add(place(shelf(1.1, 1.3, 0.35, { seed: 19, rows: 3, messy: true }), 1.1, 0, -0.9, -0.5));
    [[-0.95, 0.35], [-0.1, 0.35]].forEach(([x, z]) => g.add(place(person(true), x, 0, z)));
    for (let i = 0; i < 5; i++) {
      const b = rbox(0.28, 0.05, 0.2, matS(pick(r, [0xb85c4a, 0x4f6f9a, 0xd9b45a, 0x6f9a7a]), 0.9), 0.01);
      b.position.set(0.3 + r() * 1.1, 0.03, 0.4 + r() * 0.9); b.rotation.y = r() * 3;
      g.add(b);
    }
    for (let i = 0; i < 3; i++) {
      const b = rbox(0.24, 0.32, 0.06, matS(pick(r, [0xb85c4a, 0x4f6f9a, 0xd9b45a]), 0.9), 0.01);
      b.material = b.material.clone();
      b.userData.home = new THREE.Vector3(1.0 + i * 0.12, 0.9, -0.8);
      g.add(b);
      lost.push(b);
    }
  });
  // Stage 2: a counter and neat shelves; one person serves, one buys.
  const s2 = stage(0.5, (g) => {
    g.add(place(shelf(1.3, 1.7, 0.4, { seed: 23, rows: 4 }), -0.75, 0, -1.2));
    g.add(place(shelf(1.3, 1.7, 0.4, { seed: 24, rows: 4 }), 0.75, 0, -1.2));
    g.add(place(counter(1.6), 0, 0, 0.1));
    g.add(place(rbox(0.3, 0.2, 0.24, 0x3a4250, 0.03), 0.45, 1.05, 0.1));
    g.add(place(figure({ shirt: 0xe9e2d0, skin: 0xe8c4a4, apron: true, hair: 0x2a2220 }), 0, 0, -0.55));
    g.add(place(person(), 0.25, 0, 1.0, Math.PI));
    const tag = sign({ w: 0.9, h: 0.26, text: 'OPEN 9–5', fg: '#2f3a44', bg: '#fbf6ea', px: 100 });
    tag.position.set(0, 1.6, 0.46);
    g.add(tag);
  });
  // Stage 3: swap shelf, neat shelf, a study table with people, and the counter kept small.
  const s3 = stage(1, (g) => {
    g.add(place(shelf(1.2, 1.6, 0.4, { seed: 29, rows: 4 }), -0.9, 0, -1.25));
    const sw = shelf(1.0, 1.1, 0.35, { seed: 31, rows: 3 });
    g.add(place(sw, 0.85, 0, -1.25));
    const swS = sign({ w: 1.0, h: 0.24, text: 'SWAP · STUDY', fg: '#fff7e6', bg: '#2f6b55', px: 100 });
    swS.position.set(0.85, 1.32, -1.1);
    g.add(swS);
    g.add(place(table(1.3, 0.8), 0, 0, 0.35));
    [[-0.45, -0.25, 0], [0.45, -0.25, 0], [-0.45, 0.95, Math.PI], [0.45, 0.95, Math.PI]].forEach(([x, z, ry]) => g.add(place(person(true), x, 0, z, ry)));
    g.add(place(rbox(0.8, 0.7, 0.45, 0x9c7452, 0.04), -1.55, 0.35, 0.45));
    const lamp = new THREE.Mesh(new THREE.SphereGeometry(0.16, 12, 8), new THREE.MeshStandardMaterial({ color: 0xfff1d6, emissive: 0xffc77a, emissiveIntensity: 2.5 }));
    lamp.position.set(0, 1.7, 0.35);
    g.add(lamp);
  });
  const stages = [s1, s2, s3];

  W.addCam([
    { t: T('6a', 0.8), d: 3.4, p: [42, 17, 8], l: [17, 11, -24] },
    { t: T('6b', 0.6), d: 2.6, p: [30, 10.6, -6.5], l: [19.9, 6.9, -20.7] },
    { t: T('6c', 0.9), d: 2.8, p: [28.5, 16.8, -11], l: [14.1, 12.8, -29.3], arc: 1.5 },
    { t: T('6d', 0.7), d: 2.6, p: [31, 22.6, -6.5], l: [19.9, 18.9, -20.7] },
    { t: T('6e', 1.0), d: 2.6, p: [17.4, 46, -24.4], l: [17, 10, -25], fov: 42 },
    { t: T('6e', 7.4), d: 2.6, p: [43.4, 12.6, -43.1], l: [17, 12.4, -25], fov: 40 },
    { t: T('6f', 1.0), d: 3.0, p: [50, 17, 8], l: [17, 14.5, -24], fov: 44 },
  ]);

  // ---- labels ----
  const lbl = (g, kicker, en, zh, a, b) => L.add({ kind: 'tag', at: g, lift: 2.3, kicker, en, zh, vis: (t) => win(t, a, b) });
  lbl(s1, ['Stage 1', '阶段一'], 'Swap shelf in a dorm lounge: shared and friendly, but disorganised', '宿舍换书架：共享、友好，但杂乱无序', T('6b', 1.2), c['6c'].s + 0.6);
  lbl(s2, ['Stage 2', '阶段二'], 'A proper shop: reliable and catalogued, but the gathering is lost', '正式书店：可靠、有目录，却失去了相聚', T('6c', 2.6), c['6d'].s + 0.6);
  lbl(s3, ['Stage 3', '阶段三'], 'Swap-and-study room: sharing and gathering return, organisation stays', '换书自习空间：共享与相聚回来了，条理也保留了', T('6d', 1.6), c['6e'].s + 0.4);
  L.add({ kind: 'tag', at: s3, lift: 1.2, en: 'From above: it looks like a return', zh: '从上面看：像是回到了起点', vis: (t) => win(t, T('6e', 2.6), T('6e', 6.6)) });
  L.add({ kind: 'tag', at: at(0.75), lift: 1.0, en: 'From the side: it climbs', zh: '从侧面看：它在上升', vis: (t) => win(t, T('6e', 8.0), c['6f'].s + 0.6) });
  [[s1, '1 · shared, disorganised', '1 · 共享，但无序'], [s2, '2 · organised, no gathering', '2 · 有序，却无相聚'], [s3, '3 · shared and organised', '3 · 既共享，又有序']].forEach(([g, en, zh]) =>
    L.add({ kind: 'tag', cls: 'small plain', at: g, lift: 1.9, en, zh, vis: (t) => win(t, T('6f', 2.4), ch.e + 0.4) }));
  L.add({
    kind: 'card', cls: 'gold', screen: [0.5, 0], anchor: [0.5, 0],
    en: 'Negation of the negation: each stage overcomes the last, and keeps what was valuable in it',
    zh: '否定之否定：每个阶段克服前一阶段，又保留其中有价值的东西',
    html: '<div class="attr"><div>A way to read development, not a formula that predicts it.</div><div class="zh">这是理解发展的一种方式，不是预测发展的公式。</div></div>',
    vis: (t) => win(t, T('6f', 0.6), ch.e + 0.6),
  });

  const tmp = new THREE.Vector3();

  return {
    update(t) {
      // The helix leaves at the start of chapter 7, sinking back into the shop.
      const gone = ramp(t, c['7a'].s - 1.2, c['7a'].s + 1.2);
      const on = t > T('6a') - 0.2 && gone < 1;
      helix.visible = on;
      ghostTube.material.opacity = 0.25 * ramp(t, T('6a', 0.4), T('6a', 2.0)) * (1 - gone);
      tubeMat.opacity = 0.95 * (1 - gone);
      beyond.material.opacity = 0.45 * ramp(t, T('6e', 0.2), T('6e', 1.4)) * (1 - gone);
      const s = 0.02 + 0.48 * ramp(t, T('6c', 0.0), T('6c', 2.2)) + 0.5 * ramp(t, T('6d', 0.0), T('6d', 2.0));
      growTo(t > T('6b', 0.2) ? s : 0);

      stages.forEach((g, i) => {
        const cue = ['6b', '6c', '6d'][i];
        const k = backOut(ramp(t, T(cue, 0.2), T(cue, 1.1), (x) => x));
        scaleIn(g, on ? k * (1 - gone) : 0);
      });
      // Books go missing from the messy shelf.
      lost.forEach((b, i) => {
        const u = ramp(t, T('6b', 3.2 + i * 0.7), T('6b', 4.6 + i * 0.7));
        b.position.copy(b.userData.home).add(tmp.set(0.4 * u, 1.4 * u, 0.6 * u));
        b.rotation.set(u * 2, u * 3, 0);
        fade(b, 1 - u);
      });

      const col = win(t, T('6e', 1.6), ch.e + 0.4, 1.0, 0.8) * (1 - gone);
      column.visible = col > 0.01;
      column.children.forEach((d, i) => { d.material.opacity = 0.9 * col * (0.5 + 0.5 * Math.sin(t * 3 - i * 0.5)); });
    },
  };
}
