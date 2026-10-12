// 03 Different Tasks, Different Uses
// Three stations: understanding a concept, solving a problem, developing an
// argument. Amber marks what AI can contribute; mint marks what the learner
// must still explain, verify, or perform. Ends with a reflection pause.
import {
  THREE, MAT, COLORS, at, box, cyl, mesh, glow, std, plinth, figure, arrow,
  tube, pop, fade, rise, ramp, win, ease, camTrack, setGlow, offset, clamp01,
} from '../kit.js';

export function build(ctx) {
  const { group: G, label, overlay, cues: c, env } = ctx;
  G.add(plinth(9.4));

  function station(x, z, ry) {
    const g = at(new THREE.Group(), x, 0, z);
    g.rotation.y = ry;
    const hex = at(cyl(1.9, 1.95, 0.24, MAT.ivory2, 6), 0, 0.12, 0);
    hex.rotation.y = Math.PI / 6;
    g.add(hex);
    const ring = at(mesh(new THREE.TorusGeometry(1.75, 0.022, 8, 6), glow(COLORS.mint, 0.3), { cast: false }), 0, 0.25, 0);
    ring.rotation.x = Math.PI / 2;
    ring.rotation.z = Math.PI / 6;
    g.add(ring);
    const learner = at(figure({ seated: false, scale: 0.95 }), -1.25, 0.24, 0.85);
    learner.rotation.y = 0.6;
    g.add(learner);
    g.userData = { ring, learner };
    G.add(g);
    return g;
  }
  const S1 = station(-4.4, 0.5, 0.25), S2 = station(0, -0.7, 0), S3 = station(4.4, 0.5, -0.25);

  // --- concept: an idea crystal, AI examples orbiting, learner's explanation ribbon
  const crystal = at(mesh(new THREE.IcosahedronGeometry(0.48, 0), std(COLORS.ivory, { flatShading: true, roughness: 0.6 })), 0, 1.6, 0);
  const stand = at(cyl(0.05, 0.08, 1.0, MAT.ivory3, 12), 0, 0.74, 0);
  S1.add(crystal, stand);
  const orbiters = [0, 1, 2].map((i) => {
    const g = new THREE.Group();
    const a = at(box(0.11, 0.22, 0.22, MAT.amberGlow, 0.03), -0.055, 0, 0);
    const b = at(box(0.11, 0.22, 0.22, MAT.amberGlow, 0.03), 0.055, 0, 0);
    g.add(a, b);
    g.userData = { a, b };
    S1.add(g);
    return g;
  });
  const helix = [];
  for (let i = 0; i <= 60; i++) {
    const u = i / 60, ang = u * Math.PI * 5;
    helix.push([Math.cos(ang) * 0.72, 1.0 + u * 1.2, Math.sin(ang) * 0.72]);
  }
  const ribbon = tube(helix, 0.03, MAT.mintGlow, { segs: 200 });
  S1.add(ribbon);
  const breakSpark = at(mesh(new THREE.OctahedronGeometry(0.07), MAT.mintGlow), 0, 0, 0);
  S1.add(breakSpark);

  // --- problem: steps attempted, hinted, verified; then a similar problem alone
  const steps = [0, 1, 2, 3].map((i) => {
    const h = 0.28 + i * 0.26;
    const s = at(box(0.36, h, 0.6, MAT.ivory, 0.03), -0.6 + i * 0.4, 0.24 + h / 2, 0);
    const chk = at(box(0.2, 0.04, 0.2, MAT.mintGlow, 0.015), 0, h / 2 + 0.03, 0);
    s.add(chk);
    s.userData.chk = chk;
    S2.add(s);
    return s;
  });
  const flag = new THREE.Group();
  flag.add(at(cyl(0.012, 0.012, 0.5, MAT.ivory3, 6), 0, 0.25, 0));
  flag.add(at(box(0.22, 0.13, 0.01, MAT.mint, 0.01), 0.11, 0.42, 0));
  flag.position.set(0.6, 0.24 + 1.06, 0);
  S2.add(flag);
  const hint = arrow(0.5, MAT.amberGlow, 0.03);
  hint.rotation.z = Math.PI;
  S2.add(hint);
  const scan = at(mesh(new THREE.TorusGeometry(0.34, 0.018, 8, 48), MAT.amberGlow, { cast: false }), 0, 0, 0);
  scan.rotation.x = Math.PI / 2;
  S2.add(scan);
  const mini = at(new THREE.Group(), 1.25, 0.24, 0.85);
  for (let i = 0; i < 3; i++) {
    const h = 0.14 + i * 0.14;
    mini.add(at(box(0.2, h, 0.32, MAT.ivory, 0.02), -0.22 + i * 0.22, h / 2, 0));
  }
  S2.add(mini);

  // --- argument: an arch of claim + evidence, counterarguments, learner's keystone
  const pillars = [-0.55, 0.55].map((x) => { const p = at(cyl(0.11, 0.13, 1.1, MAT.ivory, 20), x, 0.24 + 0.55, 0); S3.add(p); return p; });
  const lintel = at(box(1.5, 0.22, 0.42, MAT.ivory, 0.04), 0, 0.24 + 1.21, 0);
  S3.add(lintel);
  const keystone = at(box(0.38, 0.3, 0.42, MAT.mintGlow, 0.05), 0, 0.24 + 1.47, 0);
  S3.add(keystone);
  const counters = [-1, 1].map((sgn) => {
    const a = arrow(0.7, MAT.amberGlow, 0.035);
    a.rotation.z = sgn * Math.PI / 2; // left arrow (sgn -1) points +x, right arrow points -x
    a.position.set(sgn * 1.65, 1.2, 0);
    S3.add(a);
    return a;
  });
  const qRings = pillars.map((p) => {
    const r = at(mesh(new THREE.TorusGeometry(0.24, 0.018, 8, 40), MAT.amberGlow, { cast: false }), p.position.x, 0.75, 0);
    r.rotation.x = Math.PI / 2;
    S3.add(r);
    return r;
  });
  const buttress = at(box(0.3, 0.8, 0.4, MAT.mint, 0.04), 1.0, 0.24 + 0.4, 0);
  buttress.rotation.z = 0.25;
  S3.add(buttress);

  // ---- labels ----
  const card = (title, ai, me) =>
    `<div class="callout"><h4>${title}</h4><div class="row ai"><span>AI can</span>${ai}</div><div class="row me"><span>The learner must</span>${me}</div></div>`;
  const L = {
    lead: overlay('<div class="lead">Different tasks ask different things of a learner</div>', 'center-top', '不同的任务，对学习者的要求也不同'),
    t1: label({ html: '<b>Understanding a concept</b>', anchor: [-4.4, 2.45, 0.5], cls: 'tag ivory', zh: '理解概念' }),
    t2: label({ html: '<b>Solving a problem</b>', anchor: [0, 2.45, -0.7], cls: 'tag ivory', zh: '解决问题' }),
    t3: label({ html: '<b>Developing an argument</b>', anchor: [4.4, 2.45, 0.5], cls: 'tag ivory', zh: '构建论证' }),
    c1: label({ html: card('Understanding a concept', 'offer examples &amp; analogies', 'explain it in their own words · notice where an analogy breaks'), anchor: [-4.4, 2.75, 0.5], cls: 'none', zh: 'AI：提供例子与类比 · 学习者：用自己的话解释，看出类比的局限' }),
    c2: label({ html: card('Solving a problem', 'give a hint · check a step', 'make the first attempt · verify each step · solve a similar problem alone'), anchor: [0, 2.75, -0.7], cls: 'none', zh: 'AI：给提示、检查步骤 · 学习者：先尝试、逐步验证、独立解决类似问题' }),
    c3: label({ html: card('Developing an argument', 'raise counterarguments · question the evidence', 'choose the claim · weigh the evidence · stand behind the conclusion'), anchor: [4.4, 2.75, 0.5], cls: 'none', zh: 'AI：提出反驳、质疑证据 · 学习者：选择论点、权衡证据、为结论负责' }),
    breaks: label({ html: 'where the analogy breaks', anchor: breakSpark, dy: -10, cls: 'tag small mint' }),
    hintL: label({ html: 'hint', anchor: hint, dy: -6, cls: 'tag small amber' }),
    alone: label({ html: 'similar problem, alone', anchor: mini, dy: -36, cls: 'tag small mint' }),
    keyL: label({ html: 'conclusion the learner stands behind', anchor: keystone, dy: -14, cls: 'tag small mint' }),
    vague: overlay('<div class="lead"><s>“Should I use AI?”</s> &nbsp;too vague</div>', 'center-top', '“我该不该用 AI？”——太笼统'),
    better: overlay('<div class="question-card"><small>A better question</small>For this task, what must I be able to do myself?</div>', 'center-mid', '对于这项任务，哪些事我必须自己能做？'),
    reflect: overlay(`<div class="reflect"><div class="ring"><svg viewBox="0 0 100 100"><circle cx="50" cy="50" r="44" class="bg"/><circle cx="50" cy="50" r="44" class="fg"/></svg></div><h3>Reflection pause</h3><p>Which part of your task could AI support?<br>Which part must stay yours?</p><small>Need longer? Press <kbd>Space</kbd> to pause.</small></div>`, 'center-mid', '你的任务中，哪一部分可以借助 AI？哪一部分必须由你自己完成？'),
  };
  const ringFg = L.reflect.el.querySelector('.fg');

  function update(lt) {
    const t = lt;
    rise(S1, ramp(t, 0.2, 2.4), 1.0);
    rise(S2, ramp(t, 0.6, 2.8), 1.0);
    rise(S3, ramp(t, 1.0, 3.2), 1.0);

    // concept
    const sB = c['3b'].s, sC = c['3c'].s;
    crystal.rotation.y = t * 0.4;
    crystal.rotation.x = Math.sin(t * 0.3) * 0.2;
    orbiters.forEach((o, i) => {
      const k = ramp(t, sC + i * 0.5, sC + 0.6 + i * 0.5);
      const ang = t * 0.7 + (i * Math.PI * 2) / 3;
      o.position.set(Math.cos(ang) * 1.0, 1.6 + Math.sin(t + i) * 0.1, Math.sin(ang) * 1.0);
      o.rotation.y = -ang;
      pop(o, k);
      const split = i === 0 ? ramp(t, sC + 5.6, sC + 6.6) : 0;
      o.userData.a.position.x = -0.055 - split * 0.1;
      o.userData.b.position.x = 0.055 + split * 0.1;
      o.userData.a.rotation.z = split * 0.3;
      o.userData.b.rotation.z = -split * 0.3;
    });
    ribbon.userData.progress(ramp(t, sC + 3.4, sC + 6.4, ease.linear));
    breakSpark.position.copy(orbiters[0].position);
    pop(breakSpark, ramp(t, sC + 5.8, sC + 6.4) * (1 - ramp(t, 52, 53)));
    breakSpark.rotation.y = t * 2;

    // problem
    const sD = c['3d'].s, sE = c['3e'].s, sF = c['3f'].s;
    steps.forEach((s, i) => {
      pop(s, i < 2 ? ramp(t, 2.4 + i * 0.2, 3.2 + i * 0.2) : ramp(t, sF + 0.4 + (i - 2) * 0.6, sF + 1.0 + (i - 2) * 0.6));
      pop(s.userData.chk, ramp(t, sF + 1.6 + i * 0.7, sF + 2.0 + i * 0.7));
    });
    pop(flag, ramp(t, sF + 1.6, sF + 2.2));
    const hk = win(t, sE + 0.2, sF + 0.6, 0.4);
    pop(hint, hk);
    hint.position.set(0.2, 1.75 + Math.sin(t * 3) * 0.05, 0);
    const sk = win(t, sE + 1.6, sE + 3.4, 0.3);
    scan.visible = sk > 0.01;
    scan.position.set(-0.6, 0.3 + clamp01((t - sE - 1.6) / 1.8) * 0.4, 0);
    if (scan.visible) fade(scan, sk);
    rise(mini, ramp(t, sF + 4.4, sF + 5.4), 0.6);

    // argument
    const sG = c['3g'].s, sH = c['3h'].s, sI = c['3i'].s;
    counters.forEach((a, i) => {
      const k = win(t, sH + i * 0.6, sI + 3.6, 0.5);
      pop(a, k);
      a.position.x = (i === 0 ? -1.65 : 1.65) + Math.sin(t * 2.4 + i) * 0.06;
    });
    qRings.forEach((r, i) => {
      const k = win(t, sH + 1.4 + i * 0.3, sI + 2.4, 0.4);
      r.visible = k > 0.01;
      if (r.visible) { fade(r, k); r.position.y = 0.6 + Math.sin(t * 2 + i) * 0.25 + 0.3; }
    });
    pop(keystone, ramp(t, sI + 3.6, sI + 4.4));
    offset(keystone, 0, (1 - ramp(t, sI + 3.6, sI + 4.6, ease.out)) * 0.8, 0);
    pop(buttress, ramp(t, sI + 1.2, sI + 2.0));
    lintel.rotation.z = Math.sin(t * 3) * 0.02 * win(t, sH, sI + 2.0, 0.5);

    // station rings: brighter when in focus
    const f1 = win(t, sB, sD, 0.6), f2 = win(t, sD, sG, 0.6), f3 = win(t, sG, c['3j'].s, 0.6);
    const after = ramp(t, c['3m'].s, c['3m'].s + 1.0);
    [S1, S2, S3].forEach((s, i) => setGlow(s.userData.ring, 0.25 + 1.3 * [f1, f2, f3][i] + 0.8 * after));

    // reflection pause: dim the room
    const sL = c['3l'].s, sM = c['3m'].s;
    env.dim = Math.max(env.dim, 0.55 * win(t, sL, sM + 0.4, 1.2));
    const pr = clamp01((t - sL) / (sM - sL));
    ringFg.style.strokeDashoffset = (276.5 * (1 - pr)).toFixed(1);

    // labels
    L.lead.set(win(t, c['3a'].s - 0.3, sB - 0.1, 0.5));
    const ov = win(t, 1.5, sB + 0.6, 0.5) + win(t, c['3j'].s - 0.4, sL, 0.5) + win(t, sM, 81, 0.5);
    L.t1.set(Math.max(ov, f1 * 0.0)); L.t2.set(ov); L.t3.set(ov);
    L.c1.set(win(t, sB + 0.8, sD - 0.2, 0.5));
    L.c2.set(win(t, sD + 0.6, sG - 0.2, 0.5));
    L.c3.set(win(t, sG + 0.6, c['3j'].s - 0.2, 0.5));
    L.breaks.set(win(t, sC + 6.0, sD, 0.4));
    L.hintL.set(win(t, sE + 0.4, sF + 0.4, 0.3));
    L.alone.set(win(t, sF + 5.0, sG, 0.4));
    L.keyL.set(win(t, sI + 4.2, c['3j'].s - 0.2, 0.4));
    L.vague.set(win(t, c['3j'].s + 0.2, c['3k'].s, 0.4));
    L.better.set(win(t, c['3k'].s, sL - 0.2, 0.5));
    L.reflect.set(win(t, sL + 0.2, sM + 0.2, 0.8));
  }

  const camera = camTrack([
    { t: 0, p: [0, 7.6, 15], l: [0, 1.2, 0] },
    { t: c['3b'].s, p: [0, 7.2, 14.2], l: [0, 1.2, 0] },
    { t: c['3b'].s + 2.2, p: [-3.8, 4.6, 9.4], l: [-4.1, 1.9, 0.4] },
    { t: c['3d'].s - 0.2, p: [-3.9, 4.5, 9.2], l: [-4.1, 1.9, 0.4] },
    { t: c['3d'].s + 1.8, p: [0.2, 4.6, 8.6], l: [0.1, 1.8, -0.7] },
    { t: c['3g'].s - 0.2, p: [0.3, 4.5, 8.4], l: [0.1, 1.8, -0.7] },
    { t: c['3g'].s + 1.8, p: [3.8, 4.6, 9.4], l: [4.1, 1.9, 0.4] },
    { t: c['3j'].s - 0.3, p: [3.9, 4.5, 9.2], l: [4.1, 1.9, 0.4] },
    { t: c['3j'].s + 2.2, p: [0, 7.0, 14.0], l: [0, 1.2, 0] },
    { t: c['3m'].s, p: [0, 6.6, 13.2], l: [0, 1.2, 0] },
    { t: 80, p: [0, 6.8, 13.6], l: [0, 1.2, 0] },
  ]);

  return { update, camera };
}
