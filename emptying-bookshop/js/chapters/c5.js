// Chapter 5 · Test, Accumulate, Revise
// Three small tests; regulars accumulate until the shop changes in kind; a setback; a revision.
import * as THREE from 'three';
import {
  win, ramp, backOut, lerp, smooth, clamp, rng, pick, PAL, seated, figure, pose, place, matS, rbox, sign,
  shelf, roundTable, stool, hedge, ringMesh, scaleIn, fade,
} from '../kit.js';

export function build({ W, L, c, chapters }) {
  const ch = chapters[4];
  const T = (id, dt = 0) => c[id].s + dt;

  W.addLight([{ t: T('5a', 2), p: 'dusk', d: 3 }, { t: T('5b', 2.5), p: 'evening', d: 3 }]);
  W.addCrowd([{ t: T('5a', 1), v: 0.4, d: 2 }]);
  W.addShop([{ t: T('5b', 1.2), door: 'OPEN\nUNTIL 9', lit: 1 }]);

  W.addCam([
    { t: T('5a', 0.8), d: 3.0, p: [21, 5.6, -11.5], l: [16.8, 1.4, -21.2] },
    { t: T('5b', 2.6), d: 2.4, p: [18.6, 5.0, -11.8], l: [14.8, 1.4, -21.0] },
    { t: T('5c', 0.6), d: 2.4, p: [22.5, 6.0, -11.0], l: [18.0, 1.2, -20.6] },
    { t: T('5d', 0.4), d: 2.4, p: [19.5, 6.4, -9.6], l: [16.2, 1.2, -20.4] },
    { t: T('5g', 0.6), d: 2.6, p: [24, 8.8, -5.5], l: [16.2, 1.2, -21] },
    { t: T('5i', 0.6), d: 2.6, p: [26, 13.5, 0.5], l: [11, 1.5, -19] },
    { t: T('5j', 0.8), d: 2.4, p: [27, 9.5, -7.5], l: [19, 1.2, -21.4] },
    { t: T('5k', 1.0), d: 3.2, p: [29, 12.5, -2.5], l: [16.5, 2, -21] },
  ]);

  W.addJia([
    { t: T('5a', 0.2), x: 16.9, z: -19.4, y: 0, ry: Math.PI },
    { t: T('5a', 1.2), x: 16.9, z: -19.4, y: 0, ry: 0.3 },
    { t: T('5b', 2.0), x: 14.9, z: -20.6, y: 0, ry: 0.2 },
    { t: T('5c', 2.2), x: 16.6, z: -18.2, y: 0, ry: 0.4 },
    { t: T('5j', 0.4), x: 16.6, z: -18.2, y: 0, ry: 0.4 },
    { t: T('5j', 3.2), x: 22.6, z: -18.4, y: 0, ry: -0.6 },
    { t: T('5k', 1.6), x: 22.6, z: -18.4, y: 0, ry: -0.4 },
    { t: T('5k', 5.0), x: 17.8, z: -18.0, y: 0, ry: 0.3 },
  ]);

  const r = rng(61);

  // ---- test 2: the swap shelf ----
  const swap = new THREE.Group();
  swap.add(shelf(1.3, 1.3, 0.42, { seed: 12, rows: 3 }));
  const swapSign = sign({ w: 1.3, h: 0.32, text: 'SWAP · A BOOK FOR A BOOK', fg: '#fff7e6', bg: '#2f6b55', px: 120 });
  swapSign.position.set(0, 1.5, 0.05);
  swap.add(swapSign);
  swap.position.set(12.8, 0, -21.3);
  W.scene.add(swap);

  // ---- test 3: a patio of borrowed tables, lights, and a sign ----
  const patio = new THREE.Group();
  patio.add(place(rbox(6.6, 0.1, 3.4, matS(0xa98763, 0.85), 0.03), 18.7, 0.05, -20.15));
  const studySign = new THREE.Group();
  studySign.add(place(rbox(0.06, 1.0, 0.06, PAL.woodDark, 0.01), -0.3, 0.5, 0));
  studySign.add(place(rbox(0.06, 1.0, 0.06, PAL.woodDark, 0.01), 0.3, 0.5, 0));
  const ss = sign({ w: 0.9, h: 0.6, text: 'STUDY\nHERE', fg: '#2f3a44', bg: '#fbf6ea', px: 160 });
  ss.position.set(0, 1.0, 0.04);
  studySign.add(ss);
  place(studySign, 15.9, 0.1, -18.5, 0.3);
  patio.add(studySign);
  // String lights between two poles.
  const bulbs = [];
  for (const x of [15.4, 22.1]) patio.add(place(rbox(0.08, 2.7, 0.08, 0x3b4250, 0.02), x, 1.35, -18.45));
  for (let i = 0; i <= 12; i++) {
    const u = i / 12;
    const b = new THREE.Mesh(new THREE.SphereGeometry(0.07, 10, 8), new THREE.MeshStandardMaterial({ color: 0xfff1d6, emissive: 0xffc77a, emissiveIntensity: 0 }));
    b.position.set(lerp(15.4, 22.1, u), 2.6 - Math.sin(u * Math.PI) * 0.45, -18.45);
    patio.add(b);
    bulbs.push(b);
  }
  W.scene.add(patio);
  const patioLight = new THREE.PointLight(0xffc98a, 0, 12, 1.5);
  patioLight.position.set(18.7, 2.6, -19.6);
  W.scene.add(patioLight);

  const tableDefs = [[16.3, -20.5], [17.9, -19.8], [19.5, -20.5], [21.1, -19.8]];
  const tables = tableDefs.map(([x, z]) => {
    const g = new THREE.Group();
    g.add(roundTable(0.48));
    for (let k = 0; k < 4; k++) {
      const a = (k / 4) * Math.PI * 2 + Math.PI / 4;
      g.add(place(stool(), Math.cos(a) * 0.72, 0, Math.sin(a) * 0.72));
      const bk = rbox(0.24, 0.04, 0.17, matS(pick(r, [0xb85c4a, 0x4f6f9a, 0xd9b45a, 0xe0ddd2]), 0.9), 0.01);
      bk.position.set(Math.cos(a) * 0.25, 0.66, Math.sin(a) * 0.25);
      bk.rotation.y = -a;
      g.add(bk);
    }
    g.position.set(x, 0.1, z);
    W.scene.add(g);
    return { g, home: new THREE.Vector3(x, 0.1, z) };
  });
  // Where the talk tables go after the revision, behind a hedge on the far side.
  const corner = [new THREE.Vector3(25.2, 0, -20.2), new THREE.Vector3(25.4, 0, -22.6)];
  const hedgeM = hedge(5.6, 1.1, 0.7);
  hedgeM.rotation.y = Math.PI / 2;
  hedgeM.position.set(23.6, 0.55, -21.2);
  W.scene.add(hedgeM);
  const timetable = sign({ w: 0.55, h: 0.75, text: 'QUIET\n6–9\n—\nTALK:\nFAR SIDE', fg: '#2f3a44', bg: '#fbf6ea', px: 200, size: 0.82 });
  timetable.position.set(13.85, 1.45, -21.84);
  W.scene.add(timetable);

  // ---- people: seats fill week by week, then a Tuesday group, then others ----
  const seats = [];
  tables.forEach((tb, ti) => {
    for (let k = 0; k < 4; k++) {
      const a = (k / 4) * Math.PI * 2 + Math.PI / 4;
      const f = seated({ shirt: pick(r, PAL.shirts), skin: pick(r, PAL.skin), hair: r() < 0.7 ? pick(r, [0x2a2220, 0x4a3426, 0x1d1d22, 0x6b4a2e]) : null });
      W.scene.add(f);
      seats.push({ f, ti, a, k });
    }
  });
  // Fill order and the time each seat is taken.
  const order = [1, 6, 9, 4, 13, 2, 7, 10, 0, 5, 8, 11, 12, 14, 15, 3];
  const times = [
    T('5d', 0.6), T('5d', 0.6), T('5d', 2.4), T('5d', 4.2), T('5d', 4.2),
    T('5e', 1.0), T('5e', 2.6), T('5e', 4.2),
    T('5f', 0.6), T('5f', 0.6), T('5f', 0.9), T('5f', 0.9), T('5f', 1.2), T('5f', 3.2), T('5f', 3.4), T('5f', 3.6),
  ];
  order.forEach((si, n) => { seats[si].t0 = times[n]; });
  const browsers = [0, 1].map((i) => {
    const f = figure({ shirt: pick(r, PAL.shirts), skin: pick(r, PAL.skin), hair: 0x2a2220 });
    place(f, 12.4 + i * 0.8, 0, -20.3, i ? -0.4 : 0.3);
    W.scene.add(f);
    return f;
  });

  // ---- the jar: each regular adds a bead; at a threshold the beads link up ----
  const jar = new THREE.Group();
  jar.position.set(11.3, 0, -18.7);
  jar.scale.setScalar(1.25);
  jar.add(place(rbox(1.2, 0.7, 1.2, matS(0x3d4555, 0.6), 0.08), 0, 0.35, 0));
  const glass = new THREE.Mesh(new THREE.CylinderGeometry(0.56, 0.56, 1.35, 32, 1, true), new THREE.MeshStandardMaterial({ color: 0xcfe6f2, transparent: true, opacity: 0.22, roughness: 0.1, side: THREE.DoubleSide, depthWrite: false }));
  glass.position.y = 1.38;
  jar.add(glass);
  jar.add(place(new THREE.Mesh(new THREE.CylinderGeometry(0.56, 0.56, 0.03, 32), new THREE.MeshStandardMaterial({ color: 0xcfe6f2, transparent: true, opacity: 0.3 })), 0, 0.72, 0));
  const NB = 46;
  const beadMat = new THREE.MeshStandardMaterial({ color: 0xffffff, roughness: 0.4, emissive: 0xffcf5a, emissiveIntensity: 0 });
  const beads = new THREE.InstancedMesh(new THREE.SphereGeometry(0.085, 12, 8), beadMat, NB);
  beads.frustumCulled = false;
  jar.add(beads);
  const rest = [], beadT = [];
  for (let i = 0; i < NB; i++) {
    const layer = Math.floor(i / 8), k = i % 8;
    const a = (k / 8) * Math.PI * 2 + layer * 0.4;
    const rr = k === 7 ? 0.05 : 0.32 + (layer % 2) * 0.06;
    rest.push(new THREE.Vector3(Math.cos(a) * rr, 0.83 + layer * 0.15, Math.sin(a) * rr));
    beadT.push(lerp(T('5d', 0.4), c['5f'].e, Math.pow(i / (NB - 1), 0.9)));
  }
  const pairs = [];
  for (let i = 0; i < NB; i++) for (let j = i + 1; j < NB; j++) if (rest[i].distanceTo(rest[j]) < 0.36) pairs.push(i, j);
  const linkGeo = new THREE.BufferGeometry();
  linkGeo.setAttribute('position', new THREE.Float32BufferAttribute(pairs.flatMap((i) => [rest[i].x, rest[i].y, rest[i].z]), 3));
  const links = new THREE.LineSegments(linkGeo, new THREE.LineBasicMaterial({ color: 0xffcf5a, transparent: true, opacity: 0, toneMapped: false }));
  jar.add(links);
  W.scene.add(jar);

  // ---- rings: the change spreading out, then noise reaching the library ----
  const pulses = [0, 1].map(() => { const m = ringMesh(1, 0.07, 0xffcf5a, 0.8); W.scene.add(m); return m; });
  const noise = [0, 1, 2, 3].map(() => { const m = ringMesh(1, 0.06, 0xff7a6b, 0.8); W.scene.add(m); return m; });

  // ---- labels ----
  L.add({ kind: 'tag', at: W.jia, lift: 1.55, en: 'From analysis to small tests', zh: '把分析变成小试验', vis: (t) => win(t, T('5a', 1.6), c['5b'].s + 0.4) });
  L.add({ kind: 'tag', at: new THREE.Vector3(15.2, 2.0, -21.9), kicker: ['Test 1', '试验一'], en: 'Open until 9', zh: '营业到晚上九点', vis: (t) => win(t, T('5b', 1.4), c['5d'].s) });
  L.add({ kind: 'tag', at: new THREE.Vector3(12.8, 1.9, -21.3), kicker: ['Test 2', '试验二'], en: 'Swap shelf: a book for a book', zh: '换书架：以书换书', anchor: [0.75, 1], vis: (t) => win(t, T('5b', 4.0), c['5d'].s) });
  L.add({ kind: 'tag', at: new THREE.Vector3(19.4, 1.5, -20.2), kicker: ['Test 3', '试验三'], en: 'Tables, lights, and a “Study here” sign', zh: '桌子、灯串和“欢迎来学习”的牌子', vis: (t) => win(t, T('5c', 2.0), c['5d'].s + 0.3) });
  const week = L.add({ kind: 'tag', cls: 'small plain', at: new THREE.Vector3(14.9, 2.7, -21.8), stem: false, en: 'Week 1', zh: '第1周', vis: (t) => win(t, T('5d', 0.3), c['5e'].s + 0.4) });
  L.add({ kind: 'tag', at: new THREE.Vector3(19.5, 1.6, -20.0), en: 'On the surface, little has changed', zh: '表面上，变化不大', vis: (t) => win(t, T('5d', 3.6), c['5e'].s + 0.4) });
  L.add({ kind: 'tag', at: jar, lift: 2.9, en: 'Adding up: people who know the shop, and their friends', zh: '在积累：熟悉这家店的人，和他们的朋友', vis: (t) => win(t, T('5e', 1.0), c['5g'].s) });
  L.add({ kind: 'tag', at: new THREE.Vector3(18.7, 1.6, -20.0), en: 'Tuesday study group', zh: '周二学习小组', vis: (t) => win(t, T('5f', 1.0), c['5g'].s + 0.4) });
  L.add({ kind: 'tag', cls: 'strike', at: new THREE.Vector3(17, 3.7, -22.3), en: 'A place you pass through', zh: '路过的地方', vis: (t) => win(t, T('5g', 3.4), c['5h'].s + 0.6) });
  L.add({ kind: 'tag', at: new THREE.Vector3(18.7, 1.6, -20.0), en: 'A place you meet', zh: '相聚的地方', vis: (t) => win(t, T('5g', 5.6), c['5h'].e) });
  L.add({ kind: 'principle', at: jar, lift: 2.9, en: 'Quantitative change → qualitative change', zh: '量变 → 质变', vis: (t) => win(t, T('5h', 0.6), c['5i'].s + 0.2) });
  L.add({ kind: 'flaw', at: new THREE.Vector3(7, 7.2, -18), en: 'Library: “Too noisy!”', zh: '图书馆：“太吵了！”', vis: (t) => win(t, T('5i', 1.6), c['5j'].s + 0.6) });
  L.add({ kind: 'flaw', at: new THREE.Vector3(17, 3.8, -22.3), en: 'Practice reveals what the analysis missed', zh: '实践暴露出分析中遗漏的问题', vis: (t) => win(t, T('5i', 3.4), c['5j'].s + 0.6) });
  L.add({ kind: 'tag', at: new THREE.Vector3(17.2, 1.5, -20.4), en: 'Quiet study by the window', zh: '窗边安静自习', vis: (t) => win(t, T('5j', 2.6), c['5k'].s + 0.4) });
  L.add({ kind: 'tag', at: new THREE.Vector3(25.3, 1.5, -21.4), en: 'Talk corner, far side', zh: '远侧讨论角', vis: (t) => win(t, T('5j', 3.6), c['5k'].s + 0.4) });
  L.add({ kind: 'tag', at: new THREE.Vector3(13.85, 1.9, -21.8), anchor: [1, 1], en: 'Timetable on the door', zh: '门上贴出时间表', vis: (t) => win(t, T('5j', 5.6), c['5k'].s + 0.4) });
  L.add({
    kind: 'card', cls: 'quote', screen: [0.5, 0.28],
    en: 'Practice, knowledge, again practice, and again knowledge.', zh: '实践、认识、再实践、再认识。',
    html: '<div class="attr"><div>Mao Zedong, “On Practice” (1937)</div><div class="zh">毛泽东《实践论》（1937年）</div></div>',
    vis: (t) => win(t, T('5k', 0.4), ch.e + 0.6, 0.7, 0.8),
  });

  const o3 = new THREE.Object3D(), tmp = new THREE.Vector3(), cold = new THREE.Color(0xdfe6ee), warm = new THREE.Color(0xffd27a), col = new THREE.Color();

  return {
    update(t) {
      const live = t > ch.s - 0.5;   // from here to the end of the film, the changes persist

      scaleIn(swap, live ? backOut(ramp(t, T('5b', 3.6), T('5b', 4.4), (x) => x)) : 0);
      const patioIn = live ? ramp(t, T('5c', 0.3), T('5c', 1.3)) : 0;
      patio.visible = patioIn > 0.01;
      patio.position.y = (1 - patioIn) * -0.3;
      const glowAmt = live ? ramp(t, T('5c', 1.0), T('5c', 2.0)) * (0.55 + 0.45 * ramp(t, T('5g', 2.4), T('5g', 4.0))) : 0;
      for (const b of bulbs) b.material.emissiveIntensity = 3 * glowAmt * (0.4 + 0.6 * (W.night ?? 0));
      patioLight.intensity = glowAmt * 9 * (0.3 + 0.7 * (W.night ?? 0));
      patioLight.color.copy(col.setHex(0xffe0b0).lerp(warm, ramp(t, T('5g', 2.4), T('5g', 4.0))));

      // Tables: in, then the two talk tables move behind the hedge.
      const move = ramp(t, T('5j', 0.6), T('5j', 2.8));
      tables.forEach((tb, i) => {
        const s = live ? backOut(ramp(t, T('5c', 0.8 + i * 0.25), T('5c', 1.6 + i * 0.25), (x) => x)) : 0;
        scaleIn(tb.g, s);
        if (i >= 2) tb.g.position.lerpVectors(tb.home, corner[i - 2], move);
        else tb.g.position.copy(tb.home);
      });
      scaleIn(hedgeM, live ? backOut(ramp(t, T('5j', 0.2), T('5j', 1.0), (x) => x)) : 0);
      hedgeM.scale.x = hedgeM.scale.z = hedgeM.scale.y;
      fade(timetable, live ? ramp(t, T('5j', 5.0), T('5j', 5.6)) : 0);

      for (const s of seats) {
        const on = live && s.t0 !== undefined ? ramp(t, s.t0, s.t0 + 0.5) : 0;
        s.f.visible = on > 0.01;
        if (!s.f.visible) continue;
        const tb = tables[s.ti].g;
        s.f.position.set(tb.position.x + Math.cos(s.a) * 0.72, 0.1 + (1 - on) * 0.4, tb.position.z + Math.sin(s.a) * 0.72);
        s.f.rotation.y = Math.atan2(-Math.cos(s.a), -Math.sin(s.a));
        s.f.scale.setScalar(on);
        const talking = s.ti >= 2 ? 1 : 0.3;
        pose(s.f, { look: Math.sin(t * (1.1 + s.k * 0.3) + s.k * 2) * 0.35 * talking, nod: 0.25 + Math.sin(t * 1.7 + s.k) * 0.08 });
      }
      browsers.forEach((f, i) => {
        f.visible = live && t > T('5e', 1.8 + i * 1.2);
        pose(f, { look: Math.sin(t * 0.8 + i) * 0.3, nod: 0.2 });
      });

      // Beads drop in one by one; at the threshold they turn gold and link into a network.
      const linked = ramp(t, T('5g', 2.6), T('5g', 4.6));
      for (let i = 0; i < NB; i++) {
        const u = live ? ramp(t, beadT[i], beadT[i] + 0.7, (x) => x * x) : 0;
        tmp.copy(rest[i]);
        tmp.y += (1 - u) * 1.6;
        o3.position.copy(tmp);
        o3.scale.setScalar(u > 0 ? 1 : 0.0001);
        o3.updateMatrix();
        beads.setMatrixAt(i, o3.matrix);
        beads.setColorAt(i, col.copy(cold).lerp(warm, linked));
      }
      beads.instanceMatrix.needsUpdate = true;
      if (beads.instanceColor) beads.instanceColor.needsUpdate = true;
      beadMat.emissiveIntensity = linked * 0.8;
      links.material.opacity = linked * 0.9;
      jar.visible = live && t > T('5d', 0.0) - 0.4;
      scaleIn(jar, 1.25 * (live ? backOut(ramp(t, T('5d', -0.4), T('5d', 0.4), (x) => x)) : 0));

      // A gold ring spreads from the shop at the change in kind.
      pulses.forEach((m, i) => {
        const u = (t - T('5g', 3.0 + i * 0.9)) / 2.6;
        m.visible = live && u > 0 && u < 1;
        if (!m.visible) return;
        const R = lerp(1.5, 16, smooth(u));
        m.scale.set(R, R, 1);
        m.position.set(17.5, 0.25, -20.5);
        m.material.opacity = 0.8 * (1 - u);
      });
      // Noise: rings reach the library; after the revision they are smaller and stop at the hedge.
      noise.forEach((m, i) => {
        const period = 1.6;
        const start = T('5i', 0.4);
        const k = (t - start - i * 0.4) / period;
        const u = k - Math.floor(k);
        const revised = t > T('5j', 2.6);
        m.visible = live && t > start + i * 0.4 && t < c['5k'].s;
        if (!m.visible) return;
        const R = revised ? lerp(0.6, 2.4, u) : lerp(0.8, 12, u);
        m.scale.set(R, R, 1);
        if (revised) m.position.set(25.3, 0.7, -21.4); else m.position.set(20.3, 0.7, -20.2);
        m.material.opacity = 0.75 * (1 - u) * (revised ? 0.7 : 1);
      });

      const wk = 1 + Math.min(2, Math.floor(clamp((t - T('5d', 0.3)) / 4.0) * 3));
      week.set(`Week ${wk}`, `第${wk}周`);
    },
  };
}
