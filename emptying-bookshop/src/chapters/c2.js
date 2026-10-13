// Chapter 2 · Three Quick Reactions
// One sweeping cause (isolated), a copied fix (no concrete analysis), giving up after a week (static).
import * as THREE from 'three';
import { win, ramp, settle, pulse, lerp, rbox, mat, sign, fade, scaleIn, backOut, arc, rng } from '../kit.js';

export function build({ W, L, c, chapters }) {
  const ch = chapters[1];

  W.addLight([{ t: c['2a'].s + 2, p: 'overcast', d: 3 }, { t: c['2j'].s + 2, p: 'day', d: 2.5 }]);
  W.addCrowd([{ t: c['2a'].s + 1, v: 0.7, d: 2 }]);
  W.addWarp(c['2h'].s, c['2j'].s + 0.4, 0);

  W.addCam([
    { t: c['2a'].s + 0.6, d: 3.2, p: [19.2, 4.0, -12.6], l: [15.4, 1.5, -21.0] },
    { t: c['2b'].s + 0.6, d: 1.8, p: [22.5, 5.4, -9.0], l: [16.2, 1.7, -19.6] },
    { t: c['2c'].s + 2.4, d: 2.6, p: [31, 24, 3], l: [15, 0, -19] },
    { t: c['2d'].s + 1.0, d: 2.6, p: [8.5, 8.2, -2.5], l: [19.8, 3.4, -20] },
    { t: c['2e'].s + 0.8, d: 2.4, p: [20.4, 4.4, -12.4], l: [16.8, 1.9, -21.6] },
    { t: c['2g'].s - 0.2, d: 2.6, p: [19.4, 3.1, -16.4], l: [17.5, 1.15, -23.6] },
    { t: c['2j'].s + 2.4, d: 3.0, p: [21, 6.5, -6], l: [14.6, 1.0, -16] },
  ]);

  const f = c['2f'].s, j = c['2j'].s;
  W.addJia([
    { t: f + 0.6, x: 14.7, z: -21.2, y: 0, ry: 0.5 },
    { t: f + 1.6, x: 14.7, z: -22.6, y: 0.12 },
    { t: f + 3.4, x: 17.4, z: -23.7, y: 0.12, ry: 0.25 },
    { t: c['2g'].s + 0.8, x: 17.4, z: -23.7, y: 0.12, ry: 0.25, nod: 0.5 },
    { t: j + 0.6, x: 17.4, z: -23.7, y: 0.12, ry: 0.25 },
    { t: j + 2.6, x: 14.7, z: -22.6, y: 0.12 },
    { t: j + 5.2, x: 14.7, z: -20.2, y: 0, ry: 0 },
  ]);

  // ---- reaction 1: a heavy verdict that blocks the view ----
  const block = new THREE.Group();
  block.add(rbox(4.6, 1.3, 0.7, mat(0xd8604f, { rough: 0.7 }), 0.08));
  const bs = sign({ w: 4.3, h: 1.0, text: 'NOBODY READS\nANYMORE', fg: '#fff7ec', bg: '#d8604f', size: 0.78 });
  bs.position.z = 0.36;
  block.add(bs);
  block.rotation.y = 0.5;
  W.scene.add(block);
  const blockAt = new THREE.Vector3(17.6, 0, -17.9);

  // ---- reaction 2: another campus as a translucent miniature, and its banner ----
  const holo = (o = 0.22) => new THREE.MeshBasicMaterial({ color: 0xcfe3ff, transparent: true, opacity: o, depthWrite: false });
  const edge = () => new THREE.LineBasicMaterial({ color: 0xe8f1ff, transparent: true, opacity: 0.75 });
  const ghost = new THREE.Group();
  ghost.position.set(24.2, 6.0, -17.0);
  const addHolo = (w, h, d, x, y, z, o) => {
    const g = new THREE.BoxGeometry(w, h, d);
    const m = new THREE.Mesh(g, holo(o));
    m.position.set(x, y, z);
    const e = new THREE.LineSegments(new THREE.EdgesGeometry(g), edge());
    e.position.copy(m.position);
    ghost.add(m, e);
  };
  addHolo(7, 0.15, 7, 0, 0, 0, 0.18);
  addHolo(0.9, 0.04, 7, 0, 0.1, 0, 0.5);            // its main path
  addHolo(1.6, 1.4, 1.6, -2.2, 0.8, -2.2);
  addHolo(1.8, 1.0, 1.2, 2.3, 0.6, 2.2);
  addHolo(1.4, 1.8, 1.4, -2.3, 1.0, 2.0);
  addHolo(1.3, 0.8, 1.0, 1.25, 0.5, -0.6, 0.45);    // its shop, right on the main path
  const crowdR = rng(31);
  const dots = [];
  for (let i = 0; i < 14; i++) {
    const d = new THREE.Mesh(new THREE.SphereGeometry(0.12, 10, 8), new THREE.MeshBasicMaterial({ color: 0xffe2b0, transparent: true, opacity: 0.9 }));
    d.userData = { a: crowdR() * 6.28, r: 0.4 + crowdR() * 0.7, s: 0.3 + crowdR() * 0.5 };
    ghost.add(d);
    dots.push(d);
  }
  W.scene.add(ghost);

  const banner = sign({ w: 3.4, h: 0.55, text: 'HALF-PRICE WEEK', fg: '#ffffff', bg: '#d8604f', size: 0.62 });
  banner.material.side = THREE.DoubleSide;
  W.scene.add(banner);
  const bannerFrom = new THREE.Vector3(25.4, 7.3, -17.6);
  const bannerTo = new THREE.Vector3(18.1, 2.2, -21.92);
  const bannerArc = arc(bannerFrom, bannerTo, 3.2);
  const tv = new THREE.Vector3();

  // ---- labels ----
  const sh = new THREE.Vector3(17, 2.0, -22.3);
  L.add({
    kind: 'flaw', at: blockAt, lift: 1.6, kicker: ['Reaction 1', '反应一'], en: '“Nobody reads anymore.”', zh: '“现在没人读书了。”',
    vis: (t) => win(t, c['2b'].s + 1.0, c['2d'].s),
  });
  L.add({
    kind: 'flaw', cls: 'stamp', at: blockAt, lift: 0.3, anchor: [1, 0.5], offset: [-150, 0], stem: false,
    en: 'Assumed, not checked', zh: '臆断，未经查证',
    vis: (t) => win(t, c['2c'].s + 0.8, c['2d'].s),
  });
  L.add({
    kind: 'flaw', cls: 'stamp', at: sh, lift: 1.4, anchor: [0, 1], offset: [30, -10], stem: false,
    en: 'Seen in isolation', zh: '孤立地看问题',
    vis: (t) => win(t, c['2c'].s + 3.6, c['2d'].s),
  });
  L.add({
    kind: 'tag', at: ghost, lift: 2.2, en: 'Another campus: its shop sits on the main path', zh: '另一所学校：书店就在主路旁',
    vis: (t) => win(t, c['2d'].s + 1.4, c['2e'].s + 0.6),
  });
  L.add({
    kind: 'flaw', at: bannerTo, lift: 0.6, kicker: ['Reaction 2', '反应二'], en: 'Copy their half-price week', zh: '照搬他们的“半价周”',
    vis: (t) => win(t, c['2d'].s + 5.6, c['2e'].s + 2.6),
  });
  L.add({
    kind: 'flaw', cls: 'stamp', at: sh, lift: 1.5, en: 'Copied, not fitted to these conditions', zh: '照搬，不合这里的条件', stem: false,
    vis: (t) => win(t, c['2e'].s + 3.0, f + 0.3),
  });
  L.add({
    kind: 'card', cls: 'quote', screen: [0.5, 0.36],
    en: 'A concrete analysis of a concrete situation', zh: '对具体情况作具体分析',
    html: '<div class="attr"><div>Lenin called it “the living soul of Marxism” (1920)</div><div class="zh">列宁称之为“马克思主义的活的灵魂”（1920年）</div></div>',
    vis: (t) => win(t, f + 0.3, c['2g'].s - 0.1, 0.7, 0.6),
  });
  L.add({
    kind: 'flaw', at: W.jia, lift: 1.5, kicker: ['Reaction 3', '反应三'], en: 'One quiet week: give up', zh: '冷清了一周：放弃',
    vis: (t) => win(t, c['2g'].s + 1.2, c['2h'].s + 1.2),
  });
  L.add({ kind: 'tag', cls: 'snapframe', screen: [0.5, 0.48], stem: false, vis: (t) => win(t, c['2h'].s, j + 0.3, 0.15, 0.5) });
  L.add({
    kind: 'flaw', cls: 'stamp', screen: [0.5, 1], anchor: [0.5, 1], en: 'A snapshot, not a process', zh: '只看一瞬，不看过程',
    vis: (t) => win(t, c['2h'].s + 1.6, c['2i'].s + 0.2),
  });
  L.add({
    kind: 'panel', cls: 'coral', screen: [0.5, 0.42], stem: false,
    html: `<h4>Metaphysical thinking<span class="zh">形而上学的思维方式</span></h4>
      <div class="chips"><span class="chip">Isolated <span class="zh">孤立</span></span><span class="chip">Static <span class="zh">静止</span></span><span class="chip">One-sided <span class="zh">片面</span></span></div>
      <div class="foot">“Metaphysical” in the Marxist sense: the opposite of dialectics.<div class="zh">这里的“形而上学”取马克思主义哲学的含义：与辩证法相对立的思维方式。</div></div>`,
    vis: (t) => win(t, c['2i'].s + 0.3, j + 0.4),
  });
  L.add({
    kind: 'principle', at: W.jia, lift: 1.55, en: 'Dialectics starts by going out to look', zh: '辩证法：从走出去看一看开始',
    vis: (t) => win(t, j + 2.2, ch.e + 2.5),
  });

  return {
    update(t) {
      // Reaction 1: the block drops, then lifts away.
      const drop = settle(ramp(t, c['2b'].s + 0.3, c['2b'].s + 1.1, (x) => x));
      const lift = ramp(t, c['2d'].s - 0.4, c['2d'].s + 0.8);
      const shown = t > c['2b'].s + 0.3 && t < c['2d'].s + 0.8;
      block.visible = shown;
      if (shown) {
        block.position.set(blockAt.x, lerp(14, 0.65, drop) + lift * 8, blockAt.z);
        block.scale.setScalar(1 - lift * 0.6);
      }
      // Isolation: everything outside the shop dims and greys.
      const iso = ramp(t, c['2c'].s + 0.6, c['2c'].s + 2.2) * (1 - ramp(t, c['2d'].s - 0.6, c['2d'].s + 0.4));
      if (iso > 0) { W.fx.focus = new THREE.Vector3(16.8, 1.2, -20.5); W.fx.focusR = 0.17; W.fx.focusAmt = iso * 0.9; }

      // Reaction 2: the other campus appears; its banner flies over and is hung on Jia's shop.
      const gIn = backOut(ramp(t, c['2d'].s + 0.6, c['2d'].s + 1.6, (x) => x));
      const gOut = 1 - ramp(t, c['2e'].s + 1.0, c['2e'].s + 2.0);
      scaleIn(ghost, gIn * gOut);
      ghost.rotation.y = -0.4 + Math.sin(t * 0.3) * 0.05;
      dots.forEach((d, i) => {
        const a = d.userData.a + t * d.userData.s;
        d.position.set(1.25 + Math.cos(a) * d.userData.r * 1.4, 0.2, -0.6 + 0.9 + Math.sin(a) * d.userData.r * 0.6 + (i % 2) * 0.2);
      });
      const fly = ramp(t, c['2d'].s + 4.2, c['2d'].s + 6.0);
      const bannerOn = t > c['2d'].s + 1.5 && t < c['2g'].s + 1.2;
      banner.visible = bannerOn;
      if (bannerOn) {
        bannerArc.getPoint(fly, tv);
        banner.position.copy(tv);
        banner.rotation.set(0, lerp(-0.4, 0, fly), Math.sin(fly * Math.PI) * 0.25 + 0.04 * fly);
        banner.scale.setScalar(lerp(0.35, 1, fly));
        fade(banner, 1 - ramp(t, c['2g'].s, c['2g'].s + 1.2));
      }

      // Reaction 3: the snapshot. A flash, colour drains, and the crowd freezes (see addWarp above).
      W.fx.flash = Math.max(W.fx.flash, pulse(t, c['2h'].s - 0.05, 0.5) * 0.8);
      const drain = win(t, c['2h'].s + 0.05, j + 0.8, 0.3, 1.2);
      W.fx.sat = Math.min(W.fx.sat, 1 - 0.88 * drain);
    },
  };
}
