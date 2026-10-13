// Chapter 7 · Your Own Bookshop
// Jia's turning points pinned to the campus, a summary, the question for the viewer, and one action.
import * as THREE from 'three';
import { win, ramp, backOut, canvasTexture, scaleIn, glow } from '../kit.js';
import { DURATION } from '../script.js';

const STEPS = [
  { at: [-5.2, 0, -6.4], en: 'Look before judging', p: 'Start from facts', zh: '先调查，再判断', pz: '从实际出发' },
  { at: [4.0, 0, 0.6], en: 'Trace the connections', p: 'Universal connection', zh: '理清各种联系', pz: '普遍联系' },
  { at: [16.2, 3.5, -25.4], en: 'Find what shapes the rest', p: 'Principal contradiction', zh: '找到制约全局的问题', pz: '主要矛盾' },
  { at: [19.6, 0, -19.6], en: 'Test ideas in practice', p: 'Practice tests knowledge', zh: '在实践中检验想法', pz: '实践检验认识' },
  { at: [11.3, 1.8, -18.7], en: 'Let small changes add up', p: 'Quantity → quality', zh: '让细小的变化积累', pz: '量变到质变' },
  { at: [17.8, 7.0, -25], en: 'Expect the process to keep moving', p: 'Negation of the negation', zh: '预料事情还会继续发展', pz: '否定之否定' },
];

export function build({ W, L, c, chapters }) {
  const ch = chapters[6];
  const T = (id, dt = 0) => c[id].s + dt;

  W.addLight([{ t: T('7a', 1.0), p: 'dusk', d: 3 }, { t: T('7d', 3.0), p: 'golden', d: 5 }]);
  W.addCrowd([{ t: T('7a', 0.5), v: 0.8, d: 3 }]);
  W.addCam([
    { t: T('7a', 1.2), d: 3.6, p: [32, 42, 40], l: [-7, 0, -9], fov: 42, push: 0.02 },
    { t: T('7d', 6.0), d: 6.0, p: [38, 36, 26], l: [-5, 0, -11], fov: 42 },
    { t: T('7e', 0.8), d: 3.4, p: [23.5, 6.6, -9], l: [17, 1.7, -21.2] },
    { t: ch.e, d: 6.5, p: [31, 17, 2], l: [15.5, 2.5, -20] },
  ]);
  W.addJia([{ t: T('7e', 0.4), x: 17.8, z: -18.0, y: 0, ry: 0.5 }]);

  // ---- numbered pins ----
  const times = [T('7b', 0.2), T('7b', 2.4), T('7c', 0.3), T('7c', 2.4), T('7c', 4.4), T('7c', 6.4)];
  const pins = STEPS.map((s, i) => {
    const g = new THREE.Group();
    const stem = new THREE.Mesh(new THREE.CylinderGeometry(0.07, 0.07, 2.6, 8), new THREE.MeshStandardMaterial({ color: 0xffcf5a, emissive: 0xffb02e, emissiveIntensity: 0.4 }));
    stem.position.y = 1.3;
    const head = new THREE.Mesh(new THREE.SphereGeometry(0.55, 20, 14), new THREE.MeshStandardMaterial({ color: 0xffcf5a, emissive: 0xffb02e, emissiveIntensity: 0.7, roughness: 0.4 }));
    head.position.y = 2.9;
    const num = canvasTexture(128, 128, (ctx, w, h) => {
      ctx.fillStyle = '#1a1406';
      ctx.font = '800 92px Inter, sans-serif';
      ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
      ctx.fillText(String(i + 1), w / 2, h / 2 + 4);
    });
    const sprite = new THREE.Sprite(new THREE.SpriteMaterial({ map: num.tex, depthTest: false, transparent: true }));
    sprite.position.y = 2.9;
    sprite.scale.setScalar(0.9);
    sprite.renderOrder = 5;
    const base = new THREE.Mesh(new THREE.CircleGeometry(0.7, 28).rotateX(-Math.PI / 2), glow(0xffcf5a, 0.35));
    base.position.y = 0.03;
    g.add(stem, head, sprite, base);
    g.position.set(...s.at);
    W.scene.add(g);
    return { g, head, t0: times[i] };
  });

  // ---- summary panel ----
  const items = STEPS.map((s, i) => `<li data-i="${i}" class="off"><span class="n">${i + 1}</span><span class="tx">${s.en} <span class="p">· ${s.p}</span><div class="zh">${s.zh}——${s.pz}</div></span></li>`).join('');
  const panel = L.add({
    kind: 'panel', cls: 'gold summary', screen: [0.03, 0.46], anchor: [0, 0.5], stem: false,
    html: `<h4>Jia's turning points<span class="zh">小嘉的转折点</span></h4><ol>${items}</ol>`,
    vis: (t) => win(t, T('7a', 1.6), c['7e'].s + 0.2, 0.8, 0.8),
  });
  const lis = [...panel.el.querySelectorAll('li')];

  L.add({
    kind: 'card', cls: 'big', screen: [0.5, 0.42],
    en: 'So: what’s the emptying bookshop in your own life?', zh: '那么，你生活中那家“日渐冷清的书店”是什么？',
    vis: (t) => win(t, T('7e', 0.3), c['7f'].s + 0.2, 0.8, 0.6),
  });
  L.add({
    kind: 'card', cls: 'gold', screen: [0.5, 0.42], kicker: ['This week', '本周试一试'],
    en: 'Pick one problem. Before you judge it, ask three people who live with it.', zh: '选一个问题。在下判断之前，先问问三个身处其中的人。',
    vis: (t) => win(t, T('7f', 0.3), c['7f'].e + 1.2, 0.7, 0.8),
  });
  L.add({
    kind: 'title', screen: [0.5, 0.45], stem: false,
    en: 'The Emptying Bookshop', zh: '日渐冷清的书店',
    html: `<div class="sub"><div>Jia, the shop and the campus are fictional. The principles are tools: test them in practice.</div>
      <div class="zh">小嘉、书店和校园都是虚构的。这些原理是工具，要在实践中检验。</div></div>`,
    vis: (t) => win(t, c['7f'].e + 1.6, DURATION + 10, 1.2, 0.1),
  });

  return {
    update(t) {
      const live = t > ch.s - 0.5;
      pins.forEach((p, i) => {
        const k = live ? backOut(ramp(t, p.t0, p.t0 + 0.7, (x) => x)) : 0;
        const out = 1 - ramp(t, T('7e', 0.0), T('7e', 1.6));
        scaleIn(p.g, k * out * 1.2);
        p.head.material.emissiveIntensity = 0.7 + 0.5 * Math.max(0, Math.sin(t * 2.4 - i * 0.6)) * ramp(t, T('7d'), T('7d', 1));
      });
      lis.forEach((li, i) => li.classList.toggle('off', t < pins[i].t0));
      W.fx.fade = Math.max(W.fx.fade, ramp(t, c['7f'].e + 0.6, c['7f'].e + 2.4) * 0.82, ramp(t, DURATION - 1.6, DURATION - 0.1));
    },
  };
}
