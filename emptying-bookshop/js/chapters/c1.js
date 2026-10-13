// Chapter 1 · An Emptying Shop
// The campus, Jia, the shop's history as a swap shelf, and a time-lapse of fewer visitors each week.
import * as THREE from 'three';
import {
  win, ramp, backOut, clamp, smooth, rbox, place, matS, sofa, shelf, seated, figure, pose, polyPath, rng, pick,
  PAL, scaleIn, arc, growTube, glow, sign,
} from '../kit.js';
import { SIDE_PATH } from '../world.js';

export function build({ W, L, c, chapters }) {
  const ch = chapters[0];

  W.addLight([{ t: 0, p: 'day', d: 1 }]);
  W.addCrowd([{ t: 0, v: 0.85, d: 1 }]);
  W.addWarp(c['1c'].s + 0.2, c['1c'].e, 4);

  W.addCam([
    { t: 0, p: [46, 40, 52], l: [3, 0, -7], fov: 38, push: 0.06 },
    { t: c['1a'].s + 3.0, d: 5.5, p: [27, 9.5, -5], l: [16, 1.8, -23] },
    { t: c['1b'].s + 1.2, d: 2.4, p: [25, 10.5, -2], l: [13.2, 3.4, -23] },
    { t: c['1c'].s + 0.9, d: 2.2, p: [24.5, 8.5, -4], l: [13.5, 0.8, -17.5] },
    { t: c['1d'].s + 1.8, d: 2.4, p: [18.6, 3.0, -14.6], l: [14.9, 1.3, -21.4] },
    { t: c['1e'].s + 1.6, d: 3.6, p: [36, 22, 12], l: [9, 0, -12] },
  ]);

  // Jia starts behind the counter, then walks to the doorway to look out.
  const d = c['1d'].s;
  W.addJia([
    { t: 0, x: 17.6, z: -23.9, y: 0.12, ry: 0.3 },
    { t: d + 0.4, x: 17.6, z: -23.9, y: 0.12, ry: 0.3 },
    { t: d + 2.6, x: 14.7, z: -22.5, y: 0.12 },
    { t: d + 4.4, x: 14.7, z: -21.2, y: 0, ry: 0.5 },
  ]);

  // ---- the shop's origin: a swap shelf in a dorm lounge, shown in a floating vignette ----
  const bubble = new THREE.Group();
  bubble.position.set(9.2, 6.6, -24.5);
  bubble.add(place(new THREE.Mesh(new THREE.CylinderGeometry(2.4, 2.25, 0.28, 48), matS(0xefe6d4, 0.85)), 0, -0.14, 0));
  bubble.add(place(new THREE.Mesh(new THREE.CylinderGeometry(1.5, 1.5, 0.02, 40), matS(0xb5694f, 0.95)), 0, 0.01, 0.2));
  bubble.add(place(sofa(0x7c8fa6), -0.4, 0, 0.45, 0.15));
  bubble.add(place(shelf(1.1, 1.3, 0.35, { seed: 9, rows: 3, messy: true }), 1.15, 0, -0.9, -0.4));
  const swap = sign({ w: 0.9, h: 0.26, text: 'FREE SWAP', fg: '#fff7e6', bg: '#b5694f', px: 120 });
  swap.position.set(1.15, 1.48, -0.7); swap.rotation.y = -0.4;
  bubble.add(swap);
  const r = rng(21);
  for (const [x, z] of [[-0.85, 0.45], [0.05, 0.5]]) {
    const f = seated({ shirt: pick(r, PAL.shirts), skin: pick(r, PAL.skin) });
    f.position.set(x, 0, z); f.rotation.y = 0.15;
    bubble.add(f);
  }
  for (let i = 0; i < 4; i++) {
    const b = rbox(0.28, 0.05, 0.2, matS(pick(r, [0xb85c4a, 0x4f6f9a, 0xd9b45a, 0x6f9a7a]), 0.9), 0.01);
    b.position.set(0.5 + r() * 0.8, 0.03, 0.3 + r() * 0.8); b.rotation.y = r() * 3;
    bubble.add(b);
  }
  W.scene.add(bubble);
  const link = growTube(arc(new THREE.Vector3(10.6, 6.2, -24.2), new THREE.Vector3(14.6, 3.7, -24.4), 1.6), 0.045, glow(0xf4efe6, 0.75), 50, 6);
  W.scene.add(link);

  // ---- visitors along the side path: a few at first, then a time-lapse that thins out ----
  const path = polyPath([[4.8, -10.4], ...SIDE_PATH.slice(1, -1), [14.6, -21.9]]);
  const len = path.getLength();
  const cs = c['1c'].s;
  const spawns = [
    ...[0, 3.6, 8.2, 13.5].map((s) => ({ t0: ch.s + s, v: 1.3 })),
    ...[0.3, 0.7, 1.05, 1.4, 1.85, 2.4, 3.1, 4.0, 5.3, 7.0].map((s) => ({ t0: cs + s, v: 7.5 })),
  ];
  const vr = rng(8);
  const visitors = spawns.map((s) => {
    const f = figure({ shirt: pick(vr, PAL.shirts), skin: pick(vr, PAL.skin), hair: vr() < 0.6 ? pick(vr, [0x2a2220, 0x4a3426, 0x1d1d22]) : null });
    W.scene.add(f);
    return { f, ...s, side: (vr() - 0.5) * 0.5 };
  });
  const tp = new THREE.Vector3(), tt = new THREE.Vector3();

  // ---- labels ----
  L.add({
    kind: 'title', screen: [0.5, 0.42], stem: false,
    en: 'The Emptying Bookshop', zh: '日渐冷清的书店',
    html: '<div class="sub"><div>How to apply Marxist principles to improve dialectical thinking skills</div><div class="zh">运用马克思主义基本原理，提升辩证思维能力</div></div>',
    vis: (t) => win(t, 0.5, c['1a'].s - 0.2, 1.2, 1.0),
  });
  L.add({
    kind: 'tag', at: W.jia, lift: 1.55, en: 'Jia runs the student second-hand bookshop', zh: '小嘉经营着学生二手书店',
    vis: (t) => win(t, c['1a'].s + 3.4, c['1b'].s + 0.8),
  });
  L.add({
    kind: 'tag', at: bubble, lift: 3.0, en: 'Years ago: a free swap shelf in a dorm lounge', zh: '几年前：宿舍休息室里的免费换书架',
    vis: (t) => win(t, c['1b'].s + 0.9, c['1c'].s + 0.4),
  });
  L.add({
    kind: 'tag', at: new THREE.Vector3(18.5, 3.6, -23), en: 'Later: a proper shop', zh: '后来：一家正式的书店',
    vis: (t) => win(t, c['1b'].s + 3.6, c['1c'].s + 0.4),
  });
  const week = L.add({
    kind: 'tag', cls: 'small plain', at: new THREE.Vector3(14.6, 2.9, -21.6), en: 'This term · week 1', zh: '本学期 · 第1周',
    vis: (t) => win(t, cs + 0.3, c['1d'].s + 0.2),
  });
  L.add({
    kind: 'tag', at: new THREE.Vector3(12.6, 0.6, -14.2), anchor: [1, 1], offset: [-10, 0],
    en: 'Fewer visitors each week', zh: '上门的人一周比一周少',
    vis: (t) => win(t, cs + 3.0, c['1d'].s + 0.6),
  });
  L.add({
    kind: 'card', screen: [0.5, 0.14], anchor: [0.5, 0], en: 'How should Jia think about the problem?', zh: '该怎样思考这个问题？',
    vis: (t) => win(t, c['1d'].s + 2.6, c['1e'].s + 0.8),
  });
  L.add({
    kind: 'note', screen: [0, 1], anchor: [0, 1], offset: [14, 0], stem: false,
    en: 'Jia, the shop and the campus are fictional.', zh: '小嘉、书店和校园都是虚构的。',
    vis: (t) => win(t, c['1e'].s + 0.4, ch.e + 0.2),
  });

  return {
    update(t) {
      if (t < 2) W.fx.fade = Math.max(W.fx.fade, 1 - ramp(t, 0.1, 1.8));

      const bIn = backOut(ramp(t, c['1b'].s + 0.2, c['1b'].s + 1.1, (x) => x));
      const bOut = 1 - ramp(t, c['1c'].s, c['1c'].s + 0.6);
      scaleIn(bubble, bIn * bOut * 1.35);
      bubble.position.y = 6.6 + Math.sin(t * 0.9) * 0.08;
      bubble.rotation.y = -0.35 + Math.sin(t * 0.25) * 0.05;
      link.userData.grow(ramp(t, c['1b'].s + 2.4, c['1b'].s + 3.6) * bOut);

      for (const v of visitors) {
        const u = (t - v.t0) * v.v / len;
        if (u <= 0 || u >= 1 || t > ch.e) { v.f.visible = false; continue; }
        v.f.visible = true;
        path.getPointAt(u, tp);
        path.getTangentAt(u, tt);
        v.f.position.set(tp.x + tt.z * v.side, 0, tp.z - tt.x * v.side);
        v.f.rotation.y = Math.atan2(tt.x, tt.z);
        v.f.scale.setScalar(smooth(u / 0.04) * smooth((1 - u) / 0.05));
        pose(v.f, { walking: 1, phase: (t - v.t0) * v.v * 4.2 });
      }

      const wk = 1 + Math.min(5, Math.floor(clamp((t - cs - 0.3) / 7.2) * 6));
      week.set(`This term · week ${wk}`, `本学期 · 第${wk}周`);
    },
  };
}
