// 06 Test, Reflect, Revise
// A cycle links independent attempt, targeted assistance, verification,
// transfer and revision. Both students try a new example with the AI off,
// then revisit it a week later. The camera pulls back to set these classroom
// checks inside the much broader idea of social practice.
import {
  THREE, MAT, COLORS, at, box, cyl, mesh, glow, std, plinth, studySpace, aiPanel, arrow, sheet,
  tube, pop, fade, rise, ramp, win, ease, camTrack, setGlow, offset, clamp01, hash, lerp,
} from '../kit.js';

export function build(ctx) {
  const { group: G, label, overlay, cues: c, env } = ctx;
  G.add(plinth(8.6));

  const R = 4.4;
  const ringPts = [];
  for (let i = 0; i <= 96; i++) {
    const a = Math.PI / 2 + (i / 96) * Math.PI * 2;
    ringPts.push([Math.cos(a) * R, 0.07, Math.sin(a) * R]);
  }
  const ring = tube(ringPts, 0.045, glow(COLORS.mint, 0.6), { segs: 240 });
  G.add(ring);

  // nodes, in travel order
  const ANG = [126, 198, 270, 342, 54].map((d) => (d * Math.PI) / 180);
  const nodePos = ANG.map((a) => new THREE.Vector3(Math.cos(a) * R, 0, Math.sin(a) * R));
  const nodes = nodePos.map((p) => {
    const g = at(new THREE.Group(), p.x, 0, p.z);
    g.add(at(cyl(0.55, 0.6, 0.3, MAT.ivory2, 40), 0, 0.15, 0));
    const top = at(mesh(new THREE.TorusGeometry(0.5, 0.03, 8, 48), glow(COLORS.mint, 0.3), { cast: false }), 0, 0.31, 0);
    top.rotation.x = Math.PI / 2;
    g.add(top);
    g.userData.top = top;
    G.add(g);
    return g;
  });
  // icons
  const icAttempt = at(new THREE.Group(), 0, 0.31, 0);
  const sh = sheet({ w: 0.36, d: 0.46, lines: 4, seed: 5 });
  sh.rotation.y = 0.3;
  const pencil = at(cyl(0.025, 0.025, 0.42, MAT.mintGlow, 10), 0.12, 0.08, 0.05);
  pencil.rotation.z = 1.2;
  icAttempt.add(sh, pencil);
  nodes[0].add(icAttempt);
  const icHelp = at(new THREE.Group(), 0, 0.31, 0);
  const beamCone = at(mesh(new THREE.ConeGeometry(0.28, 0.7, 24, 1, true), new THREE.MeshBasicMaterial({ color: COLORS.amber, transparent: true, opacity: 0.25, depthWrite: false, side: THREE.DoubleSide }), { cast: false }), 0, 0.55, 0);
  const bulb = at(mesh(new THREE.SphereGeometry(0.07, 16, 12), MAT.amberGlow, { cast: false }), 0, 0.92, 0);
  const spot = at(box(0.22, 0.12, 0.22, MAT.ivory, 0.03), 0, 0.06, 0);
  icHelp.add(beamCone, bulb, spot);
  nodes[1].add(icHelp);
  const icVerify = at(new THREE.Group(), 0, 0.75, 0);
  icVerify.add(mesh(new THREE.TorusGeometry(0.24, 0.035, 12, 48), MAT.mintGlow, { cast: false }));
  icVerify.add(mesh(new THREE.CircleGeometry(0.23, 40), MAT.glass, { cast: false }));
  icVerify.add(at(cyl(0.03, 0.03, 0.34, MAT.mint, 8), 0.2, -0.3, 0));
  icVerify.children[2].rotation.z = 0.6;
  nodes[2].add(icVerify);
  const icTransfer = at(new THREE.Group(), 0, 0.31, 0);
  icTransfer.add(at(box(0.22, 0.22, 0.22, MAT.ivory, 0.03), -0.24, 0.11, 0));
  icTransfer.add(at(mesh(new THREE.IcosahedronGeometry(0.15, 0), std(COLORS.ivory, { flatShading: true })), 0.26, 0.16, 0));
  const ta = arrow(0.28, MAT.mintGlow, 0.02);
  ta.rotation.z = -Math.PI / 2;
  ta.position.set(-0.12, 0.16, 0);
  icTransfer.add(ta);
  nodes[3].add(icTransfer);
  const icRevise = at(new THREE.Group(), 0, 0.75, 0);
  icRevise.add(mesh(new THREE.TorusGeometry(0.22, 0.035, 10, 48, Math.PI * 1.5), MAT.mintGlow, { cast: false }));
  const head = at(mesh(new THREE.ConeGeometry(0.07, 0.14, 16), MAT.mintGlow, { cast: false }), 0.22, -0.02, 0);
  icRevise.add(head);
  nodes[4].add(icRevise);
  const icons = [icAttempt, icHelp, icVerify, icTransfer, icRevise];

  const token = mesh(new THREE.SphereGeometry(0.14, 20, 16), MAT.amberGlow, { cast: false });
  G.add(token);

  // the two students at the centre, AI above
  const A = at(studySpace({ seed: 4, platform: [2.4, 0.14, 2.0], who: 'A' }), -1.35, 0, 0.1);
  const B = at(studySpace({ seed: 5, platform: [2.4, 0.14, 2.0], who: 'B' }), 1.35, 0, 0.1);
  A.rotation.y = 0.12; B.rotation.y = -0.12;
  G.add(A, B);
  const ai = at(aiPanel(), 0, 4.0, -0.9);
  ai.scale.setScalar(0.7);
  G.add(ai);
  const aiGlass = ai.children[0];
  const aiBars = ai.children.slice(6);
  const amberC = new THREE.Color(COLORS.amber), mintC = new THREE.Color(COLORS.mint), offC = new THREE.Color(0x5e6f6c);

  // work produced without AI
  function chainRow(x, n) {
    const g = at(new THREE.Group(), x, 2.05, 0.2);
    const parts = [];
    for (let i = 0; i < n; i++) {
      const b = at(box(0.5, 0.26, 0.2, MAT.ivory, 0.04), (i - (n - 1) / 2) * 0.62, 0, 0);
      b.add(at(box(0.42, 0.03, 0.02, MAT.mintGlow, 0.01), 0, 0.09, 0.105));
      g.add(b);
      parts.push(b);
    }
    G.add(g);
    return { g, parts };
  }
  const wA = chainRow(-1.5, 3), wB = chainRow(1.5, 3);

  // a week passes
  const days = [];
  for (let i = 0; i < 7; i++) {
    const d = at(box(0.26, 0.26, 0.06, MAT.ivory2, 0.03), 2.55 + (i - 3) * 0.32, 3.15, -0.8);
    G.add(d);
    days.push(d);
  }
  const dayMats = days.map((d) => { d.material = glow(COLORS.mint, 0); d.material.color.setHex(COLORS.ivory2); return d.material; });

  // social practice: a far wider ring of human activity
  const world = new THREE.Group();
  const outer = tube(Array.from({ length: 121 }, (_, i) => { const a = (i / 120) * Math.PI * 2 + Math.PI / 2; return [Math.cos(a) * 14.5, -0.46, Math.sin(a) * 14.5]; }), 0.05, glow(COLORS.ivory, 0.35), { segs: 300 });
  world.add(outer);
  const things = [];
  const thingMat = std(COLORS.ivory2, { roughness: 0.9 });
  for (let i = 0; i < 46; i++) {
    const a = (i / 46) * Math.PI * 2 + hash(i) * 0.08;
    const r = 11.2 + hash(i + 50) * 6.5;
    const kind = i % 5;
    const g = new THREE.Group();
    if (kind === 0) { g.add(at(box(0.7, 0.55, 0.6, thingMat, 0.04), 0, 0.275, 0)); g.add(at(mesh(new THREE.ConeGeometry(0.52, 0.4, 4), thingMat), 0, 0.75, 0)); g.children[1].rotation.y = Math.PI / 4; }
    else if (kind === 1) { const gear = mesh(new THREE.TorusGeometry(0.32, 0.1, 6, 10), thingMat); gear.rotation.x = Math.PI / 2; gear.position.y = 0.1; g.add(gear); }
    else if (kind === 2) { g.add(at(mesh(new THREE.ConeGeometry(0.34, 0.5, 20), thingMat), 0, 0.25, 0)); g.add(at(cyl(0.07, 0.07, 0.34, thingMat, 12), 0, 0.64, 0)); }
    else if (kind === 3) { for (let k = 0; k < 3; k++) g.add(at(box(0.18, 0.12, 1.1, thingMat, 0.03), -0.3 + k * 0.3, 0.06, 0)); }
    else { g.add(at(box(1.1, 0.08, 0.3, thingMat, 0.03), 0, 0.42, 0)); g.add(at(cyl(0.05, 0.05, 0.42, thingMat, 8), -0.45, 0.21, 0)); g.add(at(cyl(0.05, 0.05, 0.42, thingMat, 8), 0.45, 0.21, 0)); }
    g.position.set(Math.cos(a) * r, -0.5, Math.sin(a) * r);
    g.rotation.y = hash(i + 7) * Math.PI;
    g.scale.setScalar(0.9 + hash(i + 3) * 0.8);
    world.add(g);
    things.push(g);
  }
  G.add(world);
  // three recognisable kinds of practice, picked from the ring by kind and screen position
  const pick = (kind, ang) => things.filter((g, i) => i % 5 === kind)
    .reduce((best, g) => (Math.abs(Math.atan2(g.position.z, g.position.x) - ang) < Math.abs(Math.atan2(best.position.z, best.position.x) - ang) ? g : best));
  const forms = [[1, Math.PI - 0.25], [0, 0.25], [2, -2.6]].map(([k, a]) => {
    const tip = at(new THREE.Object3D(), 0, 1.25, 0); // label anchor just above the prop
    pick(k, a).add(tip);
    return tip;
  });

  // ---------- labels ----------
  const names = ['Independent attempt', 'Targeted assistance', 'Verification', 'Transfer', 'Revision'];
  const zhNames = ['独立尝试', '有针对性的帮助', '核实', '迁移', '修正'];
  const L = {
    cycle: overlay('<div class="lead">A cycle that produces evidence</div>', 'center-top', '一个能产生证据的循环'),
    nodes: nodes.map((n, i) => label({ html: `<b>${names[i]}</b>`, anchor: n, dy: 22, align: 'below', cls: `tag ${i === 1 ? 'amber' : 'mint'}`, zh: zhNames[i] })),
    off: label({ html: '<b>AI off</b>', anchor: ai, dy: -40, cls: 'tag ivory' , zh: 'AI 已关闭' }),
    newEx: label({ html: 'New example, no AI', anchor: [0, 0.4, 1.6], align: 'below', cls: 'tag ivory' }),
    example: overlay('<div class="lead"><span class="muted">New example —</span> study-group students earn higher grades</div>', 'center-top', '新例子——参加学习小组的学生成绩更高'),
    a0: label({ html: 'correlation', anchor: wA.parts[0], dy: -16, cls: 'tag tiny' }),
    a1: label({ html: '≠', anchor: wA.parts[1], dy: -16, cls: 'tag tiny' }),
    a2: label({ html: 'cause', anchor: wA.parts[2], dy: -16, cls: 'tag tiny' }),
    b0: label({ html: 'prior knowledge?', anchor: wB.parts[0], dy: -16, cls: 'tag tiny' }),
    b1: label({ html: 'effort?', anchor: wB.parts[1], dy: -16, cls: 'tag tiny' }),
    b2: label({ html: 'test: compare similar students', anchor: wB.parts[2], dx: 34, align: 'left', cls: 'tag tiny' }),
    week: label({ html: '<b>One week later</b>', anchor: days[3], dy: -18, cls: 'tag mint', zh: '一周后' }),
    faded: label({ html: 'Some held · some faded: <b>useful information</b>', anchor: [0, 2.6, 0.2], cls: 'tag ivory' }),
    evDelayed: label({ html: '<b>Delayed</b>', anchor: days[6], dx: 16, align: 'left', cls: 'tag mint' }),
    evIndep: label({ html: '<b>Independent</b>', anchor: ai, dx: -96, dy: -6, align: 'right', cls: 'tag mint' }),
    evTransfer: label({ html: '<b>Transfer</b>', anchor: nodes[3], dx: 30, dy: -40, align: 'left', cls: 'tag mint' }),
    evidence: overlay('<div class="lead">Stronger evidence <span class="muted">— still not proof</span></div>', 'center-top', '更有力的证据——但仍不是证明'),
    practice: label({ html: '<b>Social practice</b><br>changing the world, and testing ideas against it', anchor: [0, -0.4, -14.5], dy: -14, cls: 'tag ivory', zh: '社会实践：在改造世界的活动中检验认识' }),
    forms: forms.map((g, i) => label({ html: ['production', 'social life', 'scientific experiment'][i], anchor: g, dy: -10, cls: 'tag small ivory', zh: ['物质生产', '社会生活', '科学实验'][i] })),
    classroom: label({ html: '<b>Classroom checks</b><br>a small, designed part', anchor: [0, 0.1, R], dy: 10, align: 'below', cls: 'tag mint', zh: '课堂检验：其中经过设计的一小部分' }),
  };

  // token angle along the cycle, a pure function of time
  const cueStarts = ['6b', '6c', '6d', '6e', '6f'].map((id) => c[id].s);
  const loopEnd = c['6f'].e;
  function tokenAngle(t) {
    if (t < cueStarts[0]) return ANG[0];
    for (let i = 0; i < 5; i++) {
      const t0 = cueStarts[i], t1 = i < 4 ? cueStarts[i + 1] : loopEnd;
      if (t < t1) {
        const a0 = ANG[i], a1 = i < 4 ? ANG[i + 1] : ANG[0] + Math.PI * 2;
        const aa1 = a1 < a0 ? a1 + Math.PI * 2 : a1;
        // dwell at the node, then travel
        return lerp(a0, aa1, ramp(t, t0 + 0.9, t1, ease.inOut));
      }
    }
    // afterwards: slower, continuing loops
    return ANG[0] + Math.PI * 2 + (t - loopEnd) * ((Math.PI * 2) / 14);
  }
  const angDist = (a, b) => { let d = Math.abs(((a - b) % (Math.PI * 2) + Math.PI * 3) % (Math.PI * 2) - Math.PI); return d; };

  function update(lt) {
    const t = lt;
    ring.userData.progress(ramp(t, 0.8, 4.0, ease.inOut));
    nodes.forEach((n, i) => rise(n, ramp(t, 1.4 + i * 0.4, 2.6 + i * 0.4), 0.8));
    rise(A, ramp(t, 0.4, 2.4), 1.0);
    rise(B, ramp(t, 0.7, 2.7), 1.0);
    pop(ai, ramp(t, 2.0, 3.4));
    ai.userData.animate(t);
    icVerify.rotation.y = Math.sin(t * 0.8) * 0.4;
    icRevise.rotation.y = t * 0.6;

    // token around the cycle
    const ta0 = tokenAngle(t);
    const tokVis = ramp(t, cueStarts[0] - 0.8, cueStarts[0]);
    token.visible = tokVis > 0.01;
    token.position.set(Math.cos(ta0) * R, 0.45 + Math.sin(t * 3) * 0.04, Math.sin(ta0) * R);
    token.scale.setScalar(Math.max(0.001, tokVis));
    const quiet = 1 - 0.65 * win(t, c['6g'].s + 0.4, c['6j'].s, 0.8);
    setGlow(ring, 0.6 * quiet);
    nodes.forEach((n, i) => {
      const near = Math.exp(-Math.pow(angDist(ta0, ANG[i]) / 0.22, 2)) * tokVis;
      setGlow(n.userData.top, (0.3 + 2.0 * near) * quiet);
      icons[i].position.y = (i === 2 || i === 4 ? 0.75 : 0.31) + 0.12 * near;
    });

    // AI switched off
    const sG = c['6g'].s;
    const off = ramp(t, sG + 0.3, sG + 1.6);
    // switched off: the panel drains to grey and settles a little lower
    setGlow(ai.userData.ring, 1.4 * (1 - off) + 0.05);
    ai.userData.ring.material.color.copy(mintC).lerp(offC, off);
    ai.userData.frameMat.emissiveIntensity = 1.1 * (1 - off) + 0.08;
    ai.userData.frameMat.color.copy(amberC).lerp(offC, off);
    const orb = ai.userData.core.children[1].material;
    orb.emissiveIntensity = 2.0 * (1 - off) + 0.1;
    orb.color.copy(amberC).lerp(offC, off);
    aiBars.forEach((b) => setGlow(b, 0.6 * (1 - off) + 0.05));
    fade(aiGlass, 1 - 0.6 * off);
    ai.position.y = 4.0 - 1.0 * ease.inOut(off) + Math.sin(t * 0.9) * 0.05 * (1 - off);

    // their work without AI
    const sH = c['6h'].s, sI = c['6i'].s, sJ = c['6j'].s;
    wA.parts.forEach((p, i) => pop(p, ramp(t, sH + 0.4 + i * 0.7, sH + 0.9 + i * 0.7)));
    wB.parts.forEach((p, i) => pop(p, ramp(t, sI + 0.4 + i * 0.8, sI + 0.9 + i * 0.8)));

    // a week passes: day tiles light in turn, the sun swings
    days.forEach((d, i) => {
      pop(d, ramp(t, sJ - 0.4 + i * 0.08, sJ + 0.2 + i * 0.08));
      dayMats[i].emissiveIntensity = 1.2 * ramp(t, sJ + 0.2 + i * 0.32, sJ + 0.5 + i * 0.32);
    });
    env.sun = Math.sin(Math.PI * ramp(t, sJ + 0.2, sJ + 2.6, ease.inOut)) * 1.0;
    // revisit: some of it held, some faded
    const fadeK = ramp(t, sJ + 3.0, sJ + 4.2);
    const dimTo = (p, k) => { if (p.visible) fade(p, 1 - 0.7 * k); };
    dimTo(wA.parts[2], fadeK);
    dimTo(wB.parts[1], fadeK);

    // evidence → social practice
    const sK = c['6k'].s, sL = c['6l'].s;
    const ev = win(t, sK + 0.2, sL + 0.4, 0.5);
    setGlow(nodes[3].userData.top, 0.3 + 1.6 * ev * (0.6 + 0.4 * Math.sin(t * 4)));
    const kw = ramp(t, sL - 0.6, sL + 2.4);
    world.visible = kw > 0.001;
    outer.userData.progress(ramp(t, sL, sL + 3.2, ease.inOut));
    // the wider world sinks away once the narration has made its point, before the glide onward
    const out = ramp(t, c['6m'].e - 0.2, c['6m'].e + 0.5);
    things.forEach((g, i) => pop(g, ramp(t, sL + 0.3 + (i % 23) * 0.09, sL + 0.9 + (i % 23) * 0.09) * (1 - out)));
    if (out > 0) outer.userData.progress(1 - out);
    env.fogFar = lerp(0, 40, kw);

    // labels
    L.cycle.set(win(t, c['6a'].s + 2.0, cueStarts[0] + 0.4, 0.5));
    L.nodes.forEach((l, i) => l.set(win(t, cueStarts[i], sG + 0.6, 0.4)));
    L.off.set(win(t, sG + 1.0, sJ, 0.4));
    L.newEx.set(win(t, sG + 1.4, sH + 0.5, 0.4));
    L.example.set(win(t, sG + 0.8, sI + 4.6, 0.5));
    const la = win(t, sH + 1.0, sK, 0.4), lb = win(t, sI + 1.0, sK, 0.4);
    L.a0.set(la); L.a1.set(la); L.a2.set(la * (1 - 0.5 * fadeK));
    L.b0.set(lb); L.b1.set(lb * (1 - 0.5 * fadeK)); L.b2.set(lb);
    L.week.set(win(t, sJ, sJ + 3.2, 0.4));
    L.faded.set(win(t, sJ + 3.4, sK + 0.2, 0.4));
    L.evDelayed.set(win(t, sK + 0.6, sL, 0.4));
    L.evIndep.set(win(t, sK + 1.2, sL, 0.4));
    L.evTransfer.set(win(t, sK + 1.8, sL, 0.4));
    L.evidence.set(win(t, sK + 2.4, sL, 0.5));
    L.practice.set(win(t, sL + 2.2, c['6m'].e + 0.6, 0.6));
    L.forms.forEach((l, i) => l.set(win(t, sL + 3.2 + i * 0.6, c['6m'].e - 0.3, 0.5)));
    L.classroom.set(win(t, c['6m'].s, c['6m'].e + 0.6, 0.5));
  }

  const camera = camTrack([
    { t: 0, p: [0, 8.0, 14.4], l: [0, 0.9, 0] },
    { t: c['6b'].s, p: [0, 8.8, 12.6], l: [0, 0.4, 0.3] },
    { t: c['6f'].e, p: [0, 8.6, 12.4], l: [0, 0.5, 0.3] },
    { t: c['6g'].s + 1.4, p: [0, 5.4, 9.4], l: [0, 1.7, 0] },
    { t: c['6i'].e, p: [0, 5.4, 9.0], l: [0, 1.8, 0] },
    { t: c['6j'].s + 1.2, p: [0, 5.8, 9.8], l: [0, 2.3, -0.4] },
    { t: c['6k'].s + 1.0, p: [0, 7.0, 12.4], l: [0, 2.0, -0.4] },
    { t: c['6l'].s + 0.4, p: [0, 8.0, 13.4], l: [0, 0.8, 0] },
    { t: c['6l'].s + 5.5, p: [0, 19.5, 30], l: [0, -0.4, 2.0] },
    { t: 70, p: [0, 20.5, 31.5], l: [0, -0.4, 2.0] },
  ]);

  return { update, camera };
}
