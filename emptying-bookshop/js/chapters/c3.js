// Chapter 3 · Go and Find Out
// Investigation: four findings (place, time, courses, study space), each tied back to the shop by a thread.
import * as THREE from 'three';
import {
  win, ramp, backOut, lerp, smooth, rng, pick, PAL, figure, pose, place, mat, arc, growTube, scaleIn, fade, rbox,
} from '../kit.js';

export function build({ W, L, c, chapters }) {
  const ch = chapters[2];
  const T = (id, dt = 0) => c[id].s + dt;

  W.addLight([
    { t: T('3d', 3.2), p: 'golden', d: 2.5 },
    { t: T('3f', 1.8), p: 'dusk', d: 2.6 },
  ]);
  W.addCrowd([
    { t: T('3a'), v: 1.0, d: 2 },
    { t: T('3f', 1), v: 0.45, d: 2 },
  ]);
  W.addShop([
    { t: T('3b', 2.5), door: 'OUT\nASKING', lit: 0 },
    { t: T('3d', 2.0), lit: 0.6 },
    { t: T('3d', 3.2), lit: 0, door: 'CLOSED' },
    { t: T('3i'), door: 'OPEN\n9–5' },
  ]);

  W.addCam([
    { t: T('3a', 0.6), d: 3.0, p: [30, 30, 28], l: [5, 0, -9] },
    { t: T('3b', 0.8), d: 2.6, p: [17, 9, 1], l: [6, 0.8, -11.5] },
    { t: T('3b', 5.4), d: 3.2, p: [-1.2, 6.8, 4.2], l: [-5.0, 0.9, -6.4] },
    { t: T('3c', 1.0), d: 2.8, p: [32, 30, -6], l: [9, 0, -10], fov: 44 },
    { t: T('3d', 0.8), d: 2.6, p: [6, 23, 31], l: [-1, 2, -7] },
    { t: T('3e', 0.8), d: 2.6, p: [16, 15, 22], l: [-10, 2, 9] },
    { t: T('3f', 0.6), d: 2.6, p: [8, 6.4, 1.4], l: [0.5, 2.2, -11.2] },
    { t: T('3g', 0.6), d: 3.0, p: [36, 36, 26], l: [3, 0, -7] },
    { t: T('3i', 0.4), d: 3.0, p: [22.5, 6.2, -9.5], l: [13.5, 1.8, -20.5] },
  ]);

  // Jia: down the side path to the quad, then to the library plaza, then back to the shop.
  W.addJia([
    { t: 115.0, x: 14.7, z: -20.2, y: 0, ry: 0 },
    { t: T('3a', 1.2), x: 14.7, z: -20.2, y: 0, ry: 0 },
    { t: T('3a', 4.8), x: 14.6, z: -13.8 },
    { t: T('3b', 1.2), x: 11, z: -10.4 },
    { t: T('3b', 3.8), x: 4.8, z: -10.4 },
    { t: T('3b', 5.6), x: 0.9, z: -8.2 },
    { t: T('3b', 7.8), x: -4.1, z: -6.4, ry: -1.57 },
    { t: T('3e', 3.0), x: -4.1, z: -6.4, ry: -1.57 },
    { t: T('3f', 0.6), x: 2.2, z: -7.6, ry: -0.6 },
    { t: T('3g', 0.6), x: 2.2, z: -7.6, ry: -0.6 },
    { t: T('3g', 3.0), x: 4.8, z: -10.4 },
    { t: T('3g', 6.8), x: 11, z: -10.4 },
    { t: T('3h', 2.6), x: 14.6, z: -13.6 },
    { t: T('3i', 0.4), x: 15.2, z: -19.6, ry: 0.35 },
  ]);

  const r = rng(41);
  const person = (opts = {}) => figure({ shirt: pick(r, PAL.shirts), skin: pick(r, PAL.skin), hair: r() < 0.7 ? pick(r, [0x2a2220, 0x4a3426, 0x1d1d22, 0x6b4a2e]) : null, ...opts });

  // ---- the students Jia talks to on the quad ----
  const group = [[-5.8, -5.8, 1.4], [-5.7, -7.2, 1.7], [-6.7, -6.5, 1.57]].map(([x, z, ry]) => {
    const f = person({ notebook: r() < 0.5 });
    place(f, x, 0, z, ry);
    W.scene.add(f);
    return f;
  });

  // ---- threads: each finding tied back to the shop ----
  const shopPt = (dx) => new THREE.Vector3(16.6 + dx, 3.8, -22.8);
  const teal = (o) => new THREE.MeshBasicMaterial({ color: 0x4fd1c5, transparent: true, opacity: o, toneMapped: false, depthWrite: false });
  const threadDefs = [
    { key: 'place', from: new THREE.Vector3(6.8, 2.9, 3.9), to: shopPt(-0.9), lift: 7, r: 0.12, cue: '3c', at: 3.2 },
    { key: 'time', from: new THREE.Vector3(-12, 7.2, -3), to: shopPt(-0.3), lift: 7, r: 0.18, cue: '3d', at: 4.2 },
    { key: 'courses', from: new THREE.Vector3(-7, 5.9, 21.5), to: shopPt(0.3), lift: 9, r: 0.075, cue: '3e', at: 3.6 },
    { key: 'study', from: new THREE.Vector3(0.6, 2.6, -7.8), to: shopPt(0.9), lift: 6, r: 0.18, cue: '3f', at: 3.8 },
  ];
  const threads = threadDefs.map((d) => {
    const m = teal(0.85);
    const tube = growTube(arc(d.from, d.to, d.lift), d.r, m, 90, 8);
    const knot = new THREE.Mesh(new THREE.SphereGeometry(d.r * 1.0 + 0.08, 16, 12), teal(0.95));
    knot.position.copy(d.from);
    W.scene.add(tube, knot);
    return { ...d, tube, knot, m };
  });

  // ---- finding 1, place: the empty unit where the shop used to be ----
  const oldBox = new THREE.Group();
  const ob = new THREE.BoxGeometry(3.9, 2.7, 0.7);
  oldBox.add(new THREE.Mesh(ob, teal(0.16)));
  oldBox.add(new THREE.LineSegments(new THREE.EdgesGeometry(ob), new THREE.LineBasicMaterial({ color: 0x4fd1c5, transparent: true, opacity: 1, toneMapped: false })));
  oldBox.position.set(6.8, 1.4, 3.9);
  W.scene.add(oldBox);

  // ---- finding 2, time: a clock that reaches five ----
  const clock = new THREE.Group();
  clock.position.set(-12, 8.4, -3);
  const face = new THREE.Mesh(new THREE.CylinderGeometry(1.5, 1.5, 0.16, 48).rotateX(Math.PI / 2), mat(0xf7f1e4, { rough: 0.6 }));
  const rim = new THREE.Mesh(new THREE.TorusGeometry(1.5, 0.09, 10, 48), mat(0x2f3a44, { rough: 0.5 }));
  clock.add(face, rim);
  for (let i = 0; i < 12; i++) {
    const tick = rbox(0.07, i % 3 === 0 ? 0.3 : 0.18, 0.04, mat(0x2f3a44), 0.01);
    const a = (i / 12) * Math.PI * 2;
    tick.position.set(Math.sin(a) * 1.25, Math.cos(a) * 1.25, 0.1);
    tick.rotation.z = -a;
    clock.add(tick);
  }
  const hand = (len, w) => {
    const g = new THREE.Group();
    const b = rbox(w, len, 0.05, mat(0x2f3a44), 0.02);
    b.position.y = len / 2 - 0.1;
    g.add(b);
    g.position.z = 0.13;
    clock.add(g);
    return g;
  };
  const hourH = hand(0.75, 0.12), minH = hand(1.15, 0.08);
  clock.add(place(new THREE.Mesh(new THREE.SphereGeometry(0.09, 12, 8), mat(0xd8604f)), 0, 0, 0.18));
  clock.lookAt(6, 23, 31);
  W.scene.add(clock);

  // Students pouring out of the teaching buildings at five.
  const burst = [];
  for (let i = 0; i < 16; i++) {
    const fromT2 = i % 2 === 1;
    const pts = [[-18.6, fromT2 ? 7 : 0], [-17.5, fromT2 ? 7 : 0], [-17.5, 2], [i % 3 === 0 ? 0 : 6 + (i % 5) * 2.6, 2]];
    if (i % 3 === 0) pts.push([0, 12 + (i % 4)]);
    const curve = new THREE.CatmullRomCurve3(pts.map(([x, z]) => new THREE.Vector3(x, 0, z)), false, 'catmullrom', 0.05);
    const f = person();
    W.scene.add(f);
    burst.push({ f, curve, len: curve.getLength(), t0: T('3d', 3.3) + i * 0.32, v: 1.5 + r() * 0.5, side: (r() - 0.5) * 1.4 });
  }

  // ---- finding 3, courses: readings fly from the teaching buildings to the dorms ----
  const docs = [];
  for (let i = 0; i < 10; i++) {
    const g = new THREE.Group();
    const page = new THREE.Mesh(new THREE.PlaneGeometry(0.62, 0.8), new THREE.MeshBasicMaterial({ color: 0xe9fffb, side: THREE.DoubleSide, toneMapped: false, transparent: true }));
    g.add(page);
    for (let k = 0; k < 4; k++) {
      const line = new THREE.Mesh(new THREE.PlaneGeometry(0.42, 0.05), new THREE.MeshBasicMaterial({ color: 0x4fd1c5, side: THREE.DoubleSide, toneMapped: false, transparent: true }));
      line.position.set(0, 0.22 - k * 0.14, 0.005);
      g.add(line);
    }
    const from = i % 2 ? new THREE.Vector3(-22.5, 5.8, 9) : new THREE.Vector3(-22.5, 6.8, -3);
    const to = i % 3 === 0 ? new THREE.Vector3(11, 4.2, 20.6) : new THREE.Vector3(-7 + (i % 4 - 1.5) * 2, 4.2, 20.6);
    W.scene.add(g);
    docs.push({ g, curve: arc(from, to, 6 + (i % 3)), t0: T('3e', 0.6) + i * 0.42 });
  }

  // ---- finding 4, study space: groups with nowhere to sit outside a full library ----
  const waiting = [];
  [[-4.6, -8.4], [0.6, -7.6], [5.6, -8.6]].forEach(([cx, cz], gi) => {
    for (let k = 0; k < 3; k++) {
      const a = (k / 3) * Math.PI * 2 + gi;
      const f = person({ notebook: k !== 1 });
      place(f, cx + Math.cos(a) * 0.65, 0, cz + Math.sin(a) * 0.65, Math.atan2(-Math.cos(a), -Math.sin(a)));
      W.scene.add(f);
      waiting.push({ f, k, gi });
    }
  });

  // ---- labels ----
  L.add({
    kind: 'card', cls: 'quote', screen: [0.5, 0.3],
    en: 'No investigation, no right to speak.', zh: '没有调查，没有发言权。',
    html: '<div class="attr"><div>Mao Zedong, “Oppose Book Worship” (1930)</div><div class="zh">毛泽东《反对本本主义》（1930年）</div></div>',
    vis: (t) => win(t, T('3a', 0.4), c['3b'].s + 1.0, 0.7, 0.7),
  });
  L.add({
    kind: 'tag', cls: 'plain', at: W.jia, lift: 1.55, en: '“What do you actually do?”', zh: '“你们实际上是怎么做的？”',
    vis: (t) => win(t, T('3b', 7.6), c['3c'].s + 0.6),
  });
  L.add({
    kind: 'fact', at: oldBox, lift: 1.6, en: 'It used to be here, on the main path', zh: '书店原来在这里，就在主路旁',
    vis: (t) => win(t, T('3c', 2.6), c['3d'].s + 0.3),
  });
  L.add({
    kind: 'fact', at: shopPt(0), lift: 0.4, en: 'Since last year: behind the library, off the main path', zh: '去年起：在图书馆后面，离开了主路',
    vis: (t) => win(t, T('3c', 4.4), c['3d'].s + 0.3),
  });
  L.add({
    kind: 'fact', at: clock, lift: 1.8, en: 'Most classes end at 5', zh: '大多数课五点结束',
    vis: (t) => win(t, T('3d', 1.6), c['3e'].s + 0.3),
  });
  L.add({
    kind: 'fact', at: shopPt(0), lift: 0.4, en: 'The shop closes at 5', zh: '书店五点关门',
    vis: (t) => win(t, T('3d', 3.6), c['3e'].s + 0.3),
  });
  L.add({
    kind: 'fact', at: new THREE.Vector3(-22.5, 6.6, 3), en: 'Readings posted online', zh: '阅读资料放到了网上',
    vis: (t) => win(t, T('3e', 1.2), c['3f'].s + 0.3),
  });
  L.add({
    kind: 'fact', at: new THREE.Vector3(-7, 5.6, 20.5), en: 'Fewer printed books change hands', zh: '纸质书的买卖少了',
    vis: (t) => win(t, T('3e', 3.4), c['3f'].s + 0.3),
  });
  L.add({
    kind: 'fact', at: new THREE.Vector3(0.6, 1.6, -7.6), en: 'Nowhere to study together in the evening', zh: '晚上没有一起学习的地方',
    vis: (t) => win(t, T('3f', 2.8), c['3g'].s + 0.4),
  });
  L.add({
    kind: 'note', at: new THREE.Vector3(0, 6.4, -12), stem: false, en: 'The library is full, and it has to stay quiet', zh: '图书馆满座，而且必须保持安静',
    vis: (t) => win(t, T('3f', 4.4), c['3g'].s + 0.4),
  });
  [
    ['place', 'Place', '位置'], ['time', 'Time', '时间'], ['courses', 'Courses', '课程'], ['study', 'Study space', '学习空间'],
  ].forEach(([key, en, zh]) => {
    const th = threads.find((x) => x.key === key);
    L.add({ kind: 'fact', cls: 'small', at: th.from, lift: 0.5, kicker: null, en, zh, vis: (t) => win(t, T('3g', 1.6), c['3i'].s) });
  });
  L.add({
    kind: 'principle', at: shopPt(0), lift: 1.6, en: 'Universal connection: but some connections weigh more', zh: '普遍联系：但联系有强有弱、有主有次',
    vis: (t) => win(t, T('3h', 0.4), c['3i'].s + 0.6),
  });
  L.add({
    kind: 'panel', cls: 'teal', screen: [0.03, 0.4], anchor: [0, 0.5], stem: false,
    html: `<h4>What Jia found<span class="zh">小嘉的发现</span></h4><ol>
      <li><span class="n">1</span><span class="tx">Off the main path<div class="zh">不在主路旁</div></span></li>
      <li><span class="n">2</span><span class="tx">Closes at 5, just as classes end<div class="zh">五点关门，正是下课时</div></span></li>
      <li><span class="n">3</span><span class="tx">Readings are online<div class="zh">阅读资料放到了网上</div></span></li>
      <li><span class="n">4</span><span class="tx">Nowhere to study together in the evening<div class="zh">晚上没有一起学习的地方</div></span></li></ol>`,
    vis: (t) => win(t, T('3i', 0.8), ch.e + 1.2),
  });

  const tp = new THREE.Vector3(), tt = new THREE.Vector3();

  return {
    update(t) {
      const live = t > ch.s - 1 && t < ch.e + 2;

      // Students on the quad: visible while Jia is there; they turn to listen.
      group.forEach((f, i) => {
        f.visible = t > T('3a') && t < c['3f'].s + 1;
        pose(f, { look: Math.sin(t * 1.3 + i * 2) * 0.25 * ramp(t, T('3b', 7), T('3b', 8)), nod: Math.max(0, Math.sin(t * 2.2 + i)) * 0.1 });
      });

      // Threads: grow with their finding, stay for the overview, then the thick ones carry the weight.
      const emphasis = ramp(t, T('3h', 2.6), T('3h', 4.2));
      for (const th of threads) {
        const show = live && t < ch.e + 1.5;
        const g = ramp(t, T(th.cue, th.at), T(th.cue, th.at + 1.6));
        th.tube.userData.grow(show ? g : 0);
        scaleIn(th.knot, show ? backOut(ramp(t, T(th.cue, th.at - 0.3), T(th.cue, th.at + 0.4), (x) => x)) : 0);
        const weak = th.r < 0.15;
        const o = (1 - ramp(t, T('3i', -0.9), T('3i', 0.5))) * (weak ? 1 - 0.6 * emphasis : 1);
        th.m.opacity = 0.85 * o;
        th.knot.material.opacity = 0.95 * o;
      }

      // Finding 1.
      fade(oldBox, win(t, T('3c', 1.6), c['3d'].s + 0.6));
      oldBox.children[1].material.opacity = 0.65 + 0.35 * Math.sin(t * 3);

      // Finding 2: the clock sweeps from 4:40 to 5:00, then students stream out.
      const clockOn = win(t, T('3d', 0.2), c['3e'].s + 0.8);
      scaleIn(clock, backOut(clockOn) * 1.5);
      const minutes = lerp(40, 60, ramp(t, T('3d', 0.8), T('3d', 3.2)));
      minH.rotation.z = -(minutes / 60) * Math.PI * 2;
      hourH.rotation.z = -((4 + minutes / 60) / 12) * Math.PI * 2;
      for (const b of burst) {
        const u = (t - b.t0) * b.v / b.len;
        if (u <= 0 || u >= 1) { b.f.visible = false; continue; }
        b.f.visible = true;
        b.curve.getPointAt(u, tp);
        b.curve.getTangentAt(u, tt);
        b.f.position.set(tp.x + tt.z * b.side * 0.5, 0, tp.z - tt.x * b.side * 0.5);
        b.f.rotation.y = Math.atan2(tt.x, tt.z);
        b.f.scale.setScalar(smooth(u / 0.03) * smooth((1 - u) / 0.04));
        pose(b.f, { walking: 1, phase: (t - b.t0) * b.v * 4.2 });
      }

      // Finding 3: pages in flight.
      for (const d of docs) {
        const u = (t - d.t0) / 2.4;
        d.g.visible = u > 0 && u < 1;
        if (!d.g.visible) continue;
        d.curve.getPoint(smooth(u), tp);
        d.g.position.copy(tp);
        d.g.rotation.set(0.2, u * 5, Math.sin(u * 9) * 0.3);
        d.g.scale.setScalar(1.7 * Math.min(1, u * 6, (1 - u) * 5));
      }

      // Finding 4: the evening groups.
      for (const w of waiting) {
        w.f.visible = t > T('3e', 4) && t < c['3h'].s;
        pose(w.f, { look: Math.sin(t * 0.9 + w.k * 2 + w.gi) * 0.4 });
      }
    },
  };
}
