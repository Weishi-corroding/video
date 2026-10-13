// Chapter 4 · The Pull Inside the Problem
// The two sides of the shop's contradiction, the smaller problems that hang on it, and the outside condition.
import * as THREE from 'three';
import { win, ramp, settle, lerp, smooth, rng, pick, PAL, figure, pose, mat, glow, arc, growTube, rbox, sign, fade, scaleIn, backOut } from '../kit.js';

const UP = new THREE.Vector3(0, 1, 0);

/** A straight rod between two points, re-aimed every frame. */
function rod(radius, material) {
  const m = new THREE.Mesh(new THREE.CylinderGeometry(radius, radius, 1, 10, 1, true), material);
  const dir = new THREE.Vector3();
  m.userData.span = (a, b, grow = 1) => {
    dir.subVectors(b, a);
    const len = dir.length() * grow;
    m.visible = len > 0.01;
    if (!m.visible) return;
    dir.normalize();
    m.position.copy(a).addScaledVector(dir, len / 2);
    m.quaternion.setFromUnitVectors(UP, dir);
    m.scale.set(1, len, 1);
  };
  return m;
}

export function build({ W, L, c, chapters }) {
  const ch = chapters[3];
  const T = (id, dt = 0) => c[id].s + dt;

  W.addLight([{ t: T('4a', 1.5), p: 'analysis', d: 3 }]);
  W.addCrowd([{ t: T('4a', 1), v: 0.3, d: 2 }]);
  W.addShop([{ t: T('4a'), lit: 0.5, door: 'OPEN\n9–5' }, { t: T('4g', 0.6), sign: 1 }]);

  const main = { p: [17, 8.8, -3.0], l: [17, 2.4, -24], fov: 42 };
  W.addCam([
    { t: T('4a', 0.8), d: 3.0, ...main },
    { t: T('4f', 0.8), d: 2.2, p: [19.8, 6.0, -10.4], l: [17.6, 1.9, -22.2], fov: 41 },
    { t: T('4h', 0.4), d: 2.2, ...main },
    { t: T('4i', 0.8), d: 3.2, p: [36, 40, 30], l: [6, 0, -10] },
    { t: T('4j', 4.6), d: 2.8, p: [26, 15, 3], l: [15, 1.2, -20] },
  ]);

  W.addJia([
    { t: T('4a', 0.2), x: 15.2, z: -19.6, y: 0, ry: 0.35 },
    { t: T('4a', 1.6), x: 16.9, z: -19.4, y: 0, ry: Math.PI },
  ]);

  // ---- the two sides ----
  const BLUE = 0x7aa2ff, VIOLET = 0xd08cff, GOLD = 0xffcf5a;
  const pillar = (color) => {
    const g = new THREE.Group();
    g.add(rbox(0.9, 4.2, 0.9, mat(0x2c3446, { rough: 0.5 }), 0.2).translateY(2.1));
    const cap = new THREE.Mesh(new THREE.SphereGeometry(0.62, 24, 16), new THREE.MeshStandardMaterial({ color, emissive: color, emissiveIntensity: 0.55, roughness: 0.4 }));
    cap.position.y = 4.55;
    g.add(cap);
    g.userData.cap = cap;
    W.scene.add(g);
    return g;
  };
  const left = pillar(BLUE), right = pillar(VIOLET);
  const cableMat = (color) => new THREE.MeshStandardMaterial({ color, emissive: color, emissiveIntensity: 0.9, roughness: 0.4 });
  const cableL = rod(0.1, cableMat(BLUE)), cableR = rod(0.1, cableMat(VIOLET));
  W.scene.add(cableL, cableR);
  const anchorL = new THREE.Vector3(12.9, 1.9, -24.2), anchorR = new THREE.Vector3(21.1, 1.9, -24.2);

  // The arch: each side depends on the other.
  const archG = new THREE.Group();
  archG.position.x = 17;
  const archTube = growTube(arc(new THREE.Vector3(-8.4, 4.6, -25.5), new THREE.Vector3(8.4, 4.6, -25.5), 4.4), 0.07, glow(0xf4efe6, 0.85), 80, 8);
  archG.add(archTube);
  W.scene.add(archG);

  // ---- smaller problems ----
  const laptop = new THREE.Group();
  laptop.add(rbox(1.0, 0.06, 0.7, mat(0x3a4250), 0.02));
  const screen = sign({ w: 0.95, h: 0.62, text: 'www\n2009', fg: '#7d8794', bg: '#e8e4da', px: 140 });
  screen.position.set(0, 0.34, -0.33); screen.rotation.x = -0.2;
  laptop.add(screen);
  laptop.position.set(22.6, 2.3, -20.4);
  laptop.rotation.y = -0.35;
  W.scene.add(laptop);
  const small = [
    { at: new THREE.Vector3(16.0, 3.05, -21.95), en: 'Faded sign', zh: '招牌褪色', toCable: 'L', f: 0.55, ox: 0 },
    { at: new THREE.Vector3(22.6, 2.75, -20.4), en: 'Old website', zh: '网站过时', toCable: 'R', f: 0.6, ox: 0 },
    { at: new THREE.Vector3(19.8, 1.0, -21.95), en: 'Prices', zh: '价格', toCable: 'R', f: 0.3, ox: 0 },
  ].map((s) => {
    const m = rod(0.035, new THREE.MeshBasicMaterial({ color: 0xff9b8c, transparent: true, opacity: 0.9, toneMapped: false }));
    W.scene.add(m);
    const dot = new THREE.Mesh(new THREE.SphereGeometry(0.11, 12, 8), glow(0xff7a6b));
    dot.position.copy(s.at);
    W.scene.add(dot);
    return { ...s, m, dot };
  });
  const shopHook = new THREE.Vector3(17, 2.2, -23.0);

  // People drawn by the brighter sign, who look in and leave.
  const r = rng(52);
  const lookers = [0, 1, 2].map((i) => {
    const f = figure({ shirt: pick(r, PAL.shirts), skin: pick(r, PAL.skin), hair: pick(r, [0x2a2220, 0x4a3426]) });
    W.scene.add(f);
    return { f, t0: T('4g', 1.2 + i * 0.9), side: (i - 1) * 0.55 };
  });

  // ---- the outside condition ----
  const outer = new THREE.Group();
  const ringM = new THREE.Mesh(new THREE.TorusGeometry(41, 0.22, 8, 160), glow(0xcfd6e0, 0.55));
  ringM.rotation.x = -Math.PI / 2;
  outer.add(ringM);
  const arrows = [];
  for (let i = 0; i < 18; i++) {
    const a = (i / 18) * Math.PI * 2;
    const cone = new THREE.Mesh(new THREE.ConeGeometry(0.55, 1.6, 12), glow(0xe8edf4, 0.85));
    cone.userData.a = a;
    outer.add(cone);
    arrows.push(cone);
  }
  outer.position.set(0, 0.8, -2);
  W.scene.add(outer);
  const sparks = [];
  for (let i = 0; i < 18; i++) {
    const s = new THREE.Mesh(new THREE.SphereGeometry(0.28, 12, 8), glow(0xe8edf4, 1));
    const a = (i / 18) * Math.PI * 2 + 0.17;
    const from = new THREE.Vector3(Math.cos(a) * 40, 1.6, -2 + Math.sin(a) * 40);
    s.userData = { curve: arc(from, new THREE.Vector3(17, 2.2, -24), 10), t0: T('4j', 0.6 + (i % 6) * 0.45) };
    W.scene.add(s);
    sparks.push(s);
  }

  // ---- labels ----
  L.add({
    kind: 'card', screen: [0.5, 0], anchor: [0.5, 0], en: 'What pulls the shop two ways?', zh: '是什么在把书店往两个方向拉？',
    vis: (t) => win(t, T('4a', 2.2), c['4b'].s + 0.6),
  });
  L.add({
    kind: 'shop', at: left, lift: 5.4, kicker: ['One side', '一方面'], en: 'What the shop is: a daytime counter for buying and selling', zh: '书店现在：白天营业、只管买卖的柜台',
    vis: (t) => win(t, T('4b', 1.0), c['4f'].s),
  });
  L.add({
    kind: 'need', at: right, lift: 5.4, kicker: ['Other side', '另一方面'], en: 'What students now need: cheap materials, evening hours, a place to work together', zh: '学生现在需要：便宜的资料、晚上的时间、一起学习的地方',
    vis: (t) => win(t, T('4c', 1.0), c['4f'].s),
  });
  L.add({
    kind: 'tag', at: new THREE.Vector3(17, 9.2, -25.5), en: 'Each side depends on the other', zh: '双方相互依存',
    vis: (t) => win(t, T('4d', 2.0), c['4e'].s + 0.4),
  });
  L.add({
    kind: 'principle', at: new THREE.Vector3(17, 9.2, -25.5), en: 'Contradiction: a unity of opposites', zh: '矛盾：对立面的统一',
    vis: (t) => win(t, T('4e', 2.4), c['4f'].s),
  });
  small.forEach((s, i) => L.add({
    kind: 'flaw', cls: 'small', at: s.at, lift: 0.25, en: s.en, zh: s.zh,
    vis: (t) => win(t, T('4f', 2.4 + i * 1.1), c['4i'].s),
  }));
  L.add({
    kind: 'flaw', cls: 'small', at: new THREE.Vector3(14.4, 1.6, -18.8), anchor: [1, 1], offset: [-6, 0],
    en: 'They arrive, look in, and leave', zh: '来了，看一眼，又走了',
    vis: (t) => win(t, T('4g', 4.2), c['4h'].s + 0.4),
  });
  L.add({
    kind: 'principle', at: new THREE.Vector3(17, 9.2, -25.5), en: 'The principal contradiction: under these conditions, it shapes the others', zh: '主要矛盾：在这些条件下，它制约着其他问题',
    vis: (t) => win(t, T('4h', 2.2), c['4i'].s + 0.3),
  });
  L.add({
    kind: 'tag', at: new THREE.Vector3(41, 1.2, -2), kicker: ['Outside', '外部'], en: 'External condition: readings move online', zh: '外部条件：资料转到网上',
    vis: (t) => win(t, T('4i', 3.0), ch.e),
  });
  L.add({
    kind: 'card', cls: 'gold', screen: [0.5, 0], anchor: [0.5, 0], en: 'External causes work through internal causes', zh: '外因通过内因起作用',
    vis: (t) => win(t, T('4j', 0.8), ch.e + 0.4),
  });
  L.add({
    kind: 'principle', at: shopHook, lift: 2.4, en: 'Internal cause: how the shop meets its own contradiction', zh: '内因：书店如何解决自身的矛盾',
    vis: (t) => win(t, T('4j', 5.6), ch.e + 0.4),
  });

  const pL = new THREE.Vector3(), pR = new THREE.Vector3(), tmp = new THREE.Vector3(), cA = new THREE.Color(), cB = new THREE.Color();

  return {
    update(t) {
      const live = t > ch.s && t < ch.e + 1.2;
      const out = 1 - ramp(t, ch.e - 0.8, ch.e + 0.8);

      // Pillars rise, then drift apart under tension.
      const apart = ramp(t, T('4e', 0.2), T('4e', 3.0)) * 1.6;
      const riseL = settle(ramp(t, T('4b', 0.2), T('4b', 1.2), (x) => x)) * out;
      const riseR = settle(ramp(t, T('4c', 0.2), T('4c', 1.2), (x) => x)) * out;
      left.visible = live && riseL > 0.01 && out > 0.02; right.visible = live && riseR > 0.01 && out > 0.02;
      left.position.set(8.6 - apart, 0, -25.5); right.position.set(25.4 + apart, 0, -25.5);
      left.scale.set(out, Math.max(riseL, 0.01), out); right.scale.set(out, Math.max(riseR, 0.01), out);
      pL.set(left.position.x, 3.1, -25.5); pR.set(right.position.x, 3.1, -25.5);

      // Cables: blue and violet, turning gold once named as the principal contradiction.
      const gold = ramp(t, T('4h', 0.6), T('4h', 2.0));
      const strain = 1 + Math.sin(t * 9) * 0.12 * ramp(t, T('4e', 0.2), T('4e', 1.2)) * (1 - gold);
      [[cableL, anchorL, pL, BLUE, T('4b', 0.8)], [cableR, anchorR, pR, VIOLET, T('4c', 0.8)]].forEach(([m, a, b, col, t0]) => {
        m.userData.span(a, b, ramp(t, t0, t0 + 1.0) * out);
        if (!live) m.visible = false;
        m.scale.x = m.scale.z = strain * (1 + gold * 0.5);
        m.material.color.copy(cA.setHex(col).lerp(cB.setHex(GOLD), gold));
        m.material.emissive.copy(m.material.color);
        m.material.emissiveIntensity = 0.9 + gold * 0.8 + pulseAt(t, sparks) * 1.5;
      });

      archG.scale.x = (8.4 + apart) / 8.4;
      archTube.userData.grow(live ? ramp(t, T('4d', 0.4), T('4d', 2.0)) * out : 0);

      // Small problems: threads to the shop, later re-hung on the main cables.
      const reroute = ramp(t, T('4h', 1.2), T('4h', 3.0));
      fade(laptop, win(t, T('4f', 2.6), c['4i'].s + 0.6));
      laptop.position.y = 2.3 + Math.sin(t * 1.4) * 0.06;
      small.forEach((s, i) => {
        const on = win(t, T('4f', 2.2 + i * 1.1), c['4i'].s + 0.6);
        s.m.material.opacity = 0.9 * on;
        s.dot.visible = on > 0.01;
        if (!s.dot.visible) { s.m.visible = false; return; }
        tmp.lerpVectors(s.toCable === 'L' ? anchorL : anchorR, s.toCable === 'L' ? pL : pR, s.f);
        tmp.lerpVectors(shopHook, tmp, reroute);
        s.m.userData.span(s.at, tmp, ramp(t, T('4f', 2.2 + i * 1.1), T('4f', 3.0 + i * 1.1)));
      });

      for (const lk of lookers) {
        const go = ramp(t, lk.t0, lk.t0 + 3.2, (x) => x);
        const back = ramp(t, lk.t0 + 4.4, lk.t0 + 7.2, (x) => x);
        const u = go - back;
        lk.f.visible = t > lk.t0 && t < lk.t0 + 7.2;
        if (!lk.f.visible) continue;
        const z = lerp(-13.4, -20.4, smooth(u));
        lk.f.position.set(14.6 + lk.side, 0, z);
        lk.f.rotation.y = t < lk.t0 + 4.4 ? Math.PI : 0;
        const walking = (t < lk.t0 + 3.2 || t > lk.t0 + 4.4) ? 1 : 0;
        pose(lk.f, { walking, phase: t * 7, look: walking ? 0 : Math.sin((t - lk.t0) * 3) * 0.5 });
      }

      // The outside condition and the arrows that travel in to the shop.
      const ringOn = win(t, T('4i', 1.2), ch.e - 0.2, 1.2, 1.0);
      outer.visible = ringOn > 0.01;
      ringM.material.opacity = 0.55 * ringOn;
      arrows.forEach((a, i) => {
        const pulse = (Math.sin(t * 2 - i) + 1) * 0.5;
        const rr = 41 - 2.6 - pulse * 1.2;
        a.position.set(Math.cos(a.userData.a) * rr, 0.4, Math.sin(a.userData.a) * rr);
        a.rotation.set(0, 0, 0);
        a.lookAt(outer.position.x, outer.position.y, outer.position.z);
        a.rotateX(Math.PI / 2);
        a.material.opacity = 0.85 * ringOn;
        a.scale.setScalar(backOut(ramp(t, T('4i', 1.4 + i * 0.05), T('4i', 2.2 + i * 0.05), (x) => x)) || 0.0001);
      });
      for (const s of sparks) {
        const u = (t - s.userData.t0) / 3.0;
        s.visible = u > 0 && u < 1 && t < ch.e;
        if (!s.visible) continue;
        s.userData.curve.getPoint(smooth(u), s.position);
        scaleIn(s, Math.min(1, u * 5, (1 - u) * 6));
      }
    },
  };
}

// A short glow on the cables each time an outside arrow arrives.
function pulseAt(t, sparks) {
  let p = 0;
  for (const s of sparks) {
    const d = t - (s.userData.t0 + 3.0);
    if (d > 0 && d < 0.5) p = Math.max(p, 1 - d / 0.5);
  }
  return p;
}
