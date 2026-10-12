// 05 Learning Across Time
// Three linked islands: today's assignment, a later assessment, an unfamiliar
// problem. A changes strategy on the way; B finds that a convincing AI
// suggestion is wrong. Branching dashed routes show possible trajectories,
// explicitly not measured data.
import {
  THREE, MAT, COLORS, at, box, cyl, mesh, glow, std, desk, figure, card, sheet, dashedTube,
  tube, pop, fade, rise, ramp, win, ease, camTrack, setGlow, offset, clamp01, walkBob, lerp,
} from '../kit.js';

export function build(ctx) {
  const { group: G, label, overlay, cues: c } = ctx;

  const ISL = [
    { x: -5.6, y: 1.0, z: 1.4 },
    { x: 0, y: 1.3, z: -1.0 },
    { x: 5.6, y: 1.6, z: 1.4 },
  ];
  const TOP = 0.15;
  const islands = ISL.map((p) => {
    const g = at(new THREE.Group(), p.x, p.y, p.z);
    g.add(at(cyl(1.9, 1.9, 0.3, MAT.ivory2, 64), 0, 0, 0));
    const under = at(mesh(new THREE.ConeGeometry(1.88, 1.4, 9, 1), std(COLORS.ivory3, { flatShading: true })), 0, -0.85, 0);
    under.rotation.x = Math.PI;
    g.add(under);
    const shadow = at(mesh(new THREE.CircleGeometry(1.5, 48), new THREE.MeshBasicMaterial({ color: 0x041a1c, transparent: true, opacity: 0.45, depthWrite: false }), { cast: false }), 0, -0.49 - p.y, 0);
    shadow.rotation.x = -Math.PI / 2;
    g.add(shadow);
    G.add(g);
    return g;
  });
  const y1 = ISL[0].y + TOP, y2 = ISL[1].y + TOP, y3 = ISL[2].y + TOP;

  // island props
  const d1 = at(desk(), -5.2, 0, 1.0); d1.scale.setScalar(0.8); d1.position.y = y1;
  const medal = new THREE.Group();
  medal.add(mesh(new THREE.CylinderGeometry(0.3, 0.3, 0.05, 40), MAT.amberGlow));
  const medalRing = mesh(new THREE.TorusGeometry(0.36, 0.025, 8, 48), MAT.amberGlow);
  medalRing.rotation.x = Math.PI / 2;
  medal.add(medalRing);
  medal.position.set(-5.2, y1 + 1.6, 1.0);
  medal.rotation.x = Math.PI / 2 - 0.2;
  const d2 = at(desk(), 0.2, y2, -1.5); d2.scale.setScalar(0.8);
  const glass = new THREE.Group();
  const cone1 = at(mesh(new THREE.ConeGeometry(0.1, 0.16, 16), MAT.ivory), 0, 0.08, 0); cone1.rotation.x = Math.PI;
  const cone2 = at(mesh(new THREE.ConeGeometry(0.1, 0.16, 16), MAT.ivory), 0, -0.08, 0);
  glass.add(cone1, cone2);
  glass.position.set(0.55, y2 + 0.59 + 0.16, -1.5);
  const knot = at(mesh(new THREE.TorusKnotGeometry(0.34, 0.1, 120, 16, 2, 3), std(COLORS.ivory, { roughness: 0.5 })), 5.6, y3 + 1.05, 1.2);
  const knotStand = at(cyl(0.06, 0.12, 0.55, MAT.ivory3, 12), 5.6, y3 + 0.27, 1.2);
  const designRing = at(mesh(new THREE.TorusGeometry(0.75, 0.03, 8, 64), MAT.mintGlow, { cast: false }), 5.6, y3 + 0.05, 1.2);
  designRing.rotation.x = Math.PI / 2;
  G.add(d1, medal, d2, glass, knot, knotStand, designRing);

  // bridges
  function bridge(a, b, ya, yb) {
    const g = new THREE.Group();
    const curve = new THREE.CatmullRomCurve3([new THREE.Vector3(a[0], ya, a[1]), new THREE.Vector3((a[0] + b[0]) / 2, (ya + yb) / 2 + 0.12, (a[1] + b[1]) / 2), new THREE.Vector3(b[0], yb, b[1])]);
    const planks = [];
    const N = 14;
    for (let i = 0; i <= N; i++) {
      const u = i / N;
      const p = curve.getPointAt(u), tg = curve.getTangentAt(u);
      const pl = at(box(0.17, 0.05, 1.0, i % 2 ? MAT.ivory : MAT.ivory2, 0.015), p.x, p.y - 0.03, p.z);
      pl.rotation.y = Math.atan2(-tg.z, tg.x);
      g.add(pl);
      planks.push(pl);
    }
    G.add(g);
    return { g, planks, curve };
  }
  const br1 = bridge([-3.85, 0.65], [-1.75, -0.25], y1, y2);
  const br2 = bridge([1.75, -0.25], [3.85, 0.65], y2, y3);

  // students and their journeys
  function journey(points, keys) {
    const curve = new THREE.CatmullRomCurve3(points.map((p) => new THREE.Vector3(...p)));
    const last = keys[keys.length - 1];
    return (t) => {
      let u = t >= last[0] ? last[1] : keys[0][1];
      for (let i = 0; i < keys.length - 1; i++) {
        if (t >= keys[i][0] && t < keys[i + 1][0]) { u = lerp(keys[i][1], keys[i + 1][1], ease.inOut((t - keys[i][0]) / (keys[i + 1][0] - keys[i][0]))); break; }
      }
      return { u, curve };
    };
  }
  const ymid = (y1 + y2) / 2 + 0.12;
  const jA = journey(
    [[-6.1, y1, 1.9], [-4.1, y1, 0.95], [-2.8, ymid, 0.45], [-1.6, y2, 0.05], [-0.9, y2, -0.2], [1.1, y2, 0.05]],
    [[0, 0], [c['5c'].s, 0], [c['5c'].s + 3.0, 0.44], [c['5d'].s + 0.6, 0.44], [c['5d'].s + 3.0, 0.64], [c['5h'].s, 0.64], [c['5i'].e, 0.97]],
  );
  const jB = journey(
    [[-5.0, y1, 0.9], [-3.9, y1, 0.4], [-2.8, ymid, -0.05], [-1.6, y2, -0.5], [0.5, y2, -0.75], [1.6, y2, -0.85]],
    [[0, 0], [c['5b'].s + 0.6, 0], [c['5c'].s + 3.2, 0.78], [c['5h'].s, 0.78], [c['5i'].e, 0.97]],
  );
  const figA = figure({ seated: false }), figB = figure({ seated: false });
  G.add(figA, figB);
  const orbA = at(mesh(new THREE.SphereGeometry(0.065, 16, 12), MAT.amberGlow, { cast: false }), 0.36, 1.12, -0.12);
  const orbB = at(mesh(new THREE.SphereGeometry(0.065, 16, 12), MAT.amberGlow, { cast: false }), 0.36, 1.12, -0.12);
  figA.add(orbA); figB.add(orbB);
  // A's finished answer (dropped), draft (kept) and hint spark
  const ansA = card({ w: 0.38, h: 0.5, accent: MAT.amberGlow });
  G.add(ansA);
  const draftA = sheet({ w: 0.3, d: 0.38, lines: 4, lineMat: MAT.mintGlow, seed: 11 });
    G.add(draftA);
  const spark = at(mesh(new THREE.OctahedronGeometry(0.06), MAT.amberGlow, { cast: false }), 0, 0, 0);
  G.add(spark);
  // B's convincing suggestion and the verification lens
  const sugg = new THREE.Group();
  const sL = at(new THREE.Group(), -0.13, 0, 0), sR = at(new THREE.Group(), 0.13, 0, 0);
  const ornate = std(COLORS.amber, { roughness: 0.3, metalness: 0.2, emissive: COLORS.amber, emissiveIntensity: 0.8 });
  sL.add(at(box(0.26, 0.62, 0.03, MAT.paper, 0.01), 0, 0, 0), at(box(0.26, 0.05, 0.04, ornate, 0.01), 0, 0.27, 0));
  sR.add(at(box(0.26, 0.62, 0.03, MAT.paper, 0.01), 0, 0, 0), at(box(0.26, 0.05, 0.04, ornate, 0.01), 0, 0.27, 0));
  const seal = at(mesh(new THREE.TorusGeometry(0.07, 0.018, 8, 32), ornate), -0.13, -0.18, 0.03);
  sR.add(at(mesh(new THREE.CylinderGeometry(0.05, 0.05, 0.02, 24), ornate), 0.0, -0.18, 0.03));
  sR.children[sR.children.length - 1].rotation.x = Math.PI / 2;
  sR.add(seal);
  for (let i = 0; i < 4; i++) {
    sL.add(at(box(0.18, 0.02, 0.01, MAT.ivory3, 0.004), 0.02, 0.15 - i * 0.07, 0.02));
    sR.add(at(box(0.16, 0.02, 0.01, MAT.ivory3, 0.004), -0.02, 0.15 - i * 0.07, 0.02));
  }
  sugg.add(sL, sR);
  const suggPos = new THREE.Vector3(1.1, y2 + 1.55, -0.2);
  sugg.position.copy(suggPos);
  G.add(sugg);
  const lens = new THREE.Group();
  lens.add(mesh(new THREE.TorusGeometry(0.2, 0.03, 12, 48), MAT.mintGlow, { cast: false }));
  const lensGlass = mesh(new THREE.CircleGeometry(0.19, 40), MAT.glass, { cast: false });
  lens.add(lensGlass);
  lens.add(at(cyl(0.022, 0.022, 0.3, MAT.mint, 8), 0.16, -0.27, 0));
  lens.children[2].rotation.z = 0.6;
  G.add(lens);

  // trajectories: branching routes, deliberately without axes or numbers
  const H = 0.4;
  const routes = {
    a0: dashedTube([[-0.9, y2 + H, -0.1], [0.3, y2 + H, 0.05], [1.3, y2 + H, 0.15]], COLORS.ivory, { emissive: 0.5 }),
    a1: dashedTube([[1.3, y2 + H, 0.15], [2.8, (y2 + y3) / 2 + H + 0.1, 0.35], [4.6, y3 + H, 0.95], [6.0, y3 + H + 0.1, 1.0], [7.6, y3 + H + 0.3, 0.6]], COLORS.mint),
    a2: dashedTube([[1.3, y2 + H, 0.15], [2.1, y2 + H, 1.1], [2.6, y2 - 0.2, 2.2], [2.9, -0.2, 3.1]], COLORS.ivory3, { emissive: 0.25 }),
    b0: dashedTube([[0.5, y2 + H, -0.8], [1.2, y2 + H, -0.85], [1.9, y2 + H, -0.9]], COLORS.ivory, { emissive: 0.5 }),
    b1: dashedTube([[1.9, y2 + H, -0.9], [3.2, (y2 + y3) / 2 + H + 0.1, -0.2], [4.8, y3 + H, 0.5], [6.2, y3 + H + 0.1, 0.4], [7.8, y3 + H + 0.3, -0.1]], COLORS.mint),
    b2: dashedTube([[1.9, y2 + H, -0.9], [2.6, y2 + H, -1.9], [3.0, y2 - 0.1, -2.9], [3.2, -0.2, -3.7]], COLORS.amber, { emissive: 0.45 }),
  };
  Object.values(routes).forEach((r) => G.add(r));
  const nodes = [[1.3, y2 + H, 0.15], [1.9, y2 + H, -0.9]].map((p) => { const n = at(mesh(new THREE.SphereGeometry(0.07, 16, 12), MAT.mintGlow, { cast: false }), ...p); G.add(n); return n; });

  // ---------- labels ----------
  const L = {
    i1: label({ html: '<b>Today’s assignment</b>', anchor: [ISL[0].x, ISL[0].y - 0.2, ISL[0].z + 1.9], align: 'below', cls: 'tag ivory', zh: '今天的作业' }),
    i2: label({ html: '<b>A later assessment</b>', anchor: [ISL[1].x, ISL[1].y - 0.2, ISL[1].z + 1.9], align: 'below', cls: 'tag ivory', zh: '之后的测评' }),
    i3: label({ html: '<b>An unfamiliar problem</b>', anchor: [ISL[2].x, ISL[2].y - 0.2, ISL[2].z + 1.9], align: 'below', cls: 'tag ivory', zh: '陌生的问题' }),
    time: overlay('<div class="time-arrow"><span>time</span></div>', 'bottom-band'),
    strong: label({ html: 'a strong result here…', anchor: medal, dy: -24, cls: 'tag small amber' }),
    q2: label({ html: '?', anchor: [0, y2 + 1.9, -1.0], cls: 'qmark' }),
    q3: label({ html: '?', anchor: [5.6, y3 + 2.1, 1.2], cls: 'qmark' }),
    tagA: label({ html: '<b>A</b>', anchor: figA.userData.head, dy: -28, cls: 'tag ivory small' }),
    tagB: label({ html: '<b>B</b>', anchor: figB.userData.head, dy: -28, cls: 'tag ivory small' }),
    stratA: label({ html: '<b>A changes strategy</b><br>attempt first · ask for a hint, not an answer', anchor: [-2.8, ymid + 1.5, 0.45], cls: 'tag mint', zh: 'A 改变策略：先尝试，再要提示而不是答案' }),
    suggB: label({ html: '<b>Convincing AI suggestion</b><br>“A study proves AI use causes better learning.”', anchor: sugg, dx: 30, align: 'left', cls: 'tag amber', zh: '看似可信的 AI 建议：“一项研究证明使用 AI 会带来更好的学习。”' }),
    wrongB: label({ html: '<b>B checks the source</b><br>no such study · correlation treated as cause', anchor: sugg, dx: 30, dy: 10, align: 'left', cls: 'tag mint', zh: 'B 核查来源：研究不存在 · 把相关当成了因果' }),
    habit: label({ html: 'Good habits one day ≠ good judgment the next', anchor: [0.6, y2 + 2.6, -1.0], cls: 'tag ivory' }),
    disclaimer: overlay('<div class="lead">Possible trajectories <span class="muted">— illustrative, not measured data</span></div>', 'center-top', '可能的轨迹——仅为示意，并非测量数据'),
    ra1: label({ html: 'keeps attempting first', anchor: [7.6, y3 + H + 0.3, 0.6], dy: 8, align: 'below', cls: 'tag tiny mint' }),
    ra2: label({ html: 'drifts back to finished answers', anchor: [2.8, y2 - 0.1, 2.8], align: 'below', cls: 'tag tiny' }),
    rb1: label({ html: 'keeps checking sources', anchor: [7.8, y3 + H + 0.3, -0.1], dy: -8, cls: 'tag tiny mint' }),
    rb2: label({ html: 'trusts fluent output again', anchor: [3.1, y2 + 0.1, -3.2], dx: 10, align: 'left', cls: 'tag tiny amber' }),
    design: label({ html: 'how the next task is designed', anchor: [5.6, y3 + 0.05, 2.2], dy: 6, align: 'below', cls: 'tag small mint' }),
    open: label({ html: 'Neither student is finished', anchor: [3.0, y2 + 2.4, 0], cls: 'tag ivory' }),
  };

  const tmp = new THREE.Vector3(), tmp2 = new THREE.Vector3(), tg = new THREE.Vector3();
  const dimC = new THREE.Color(COLORS.ivory3);
  function place(fig, j, t) {
    const { u, curve } = j(t);
    curve.getPointAt(clamp01(u), fig.position);
    curve.getTangentAt(clamp01(u), tg);
    fig.rotation.y = Math.atan2(tg.x, tg.z);
    const moving = Math.abs(j(t + 0.05).u - u) > 1e-4;
    walkBob(fig, t, moving ? 1 : 0);
    return u;
  }

  function update(lt) {
    const t = lt;
    islands.forEach((g, i) => rise(g, ramp(t, 0.3 + i * 0.5, 2.6 + i * 0.5), 1.6));
    const bk1 = ramp(t, 2.2, 4.2, ease.linear), bk2 = ramp(t, 2.8, 4.8, ease.linear);
    br1.planks.forEach((p, i) => pop(p, clamp01(bk1 * br1.planks.length - i)));
    br2.planks.forEach((p, i) => pop(p, clamp01(bk2 * br2.planks.length - i)));
    pop(d1, ramp(t, 1.4, 2.2)); pop(d2, ramp(t, 1.8, 2.6)); pop(glass, ramp(t, 2.2, 2.8));
    pop(knot, ramp(t, 2.6, 3.4)); pop(knotStand, ramp(t, 2.4, 3.0));
    knot.rotation.y = t * 0.5; knot.rotation.x = 0.3;
    glass.rotation.z = Math.sin(t * 0.8) * 0.15;

    // the grade medal on island 1
    const sB = c['5b'].s;
    pop(medal, win(t, sB - 0.4, c['5c'].s + 1.5, 0.6));
    medal.rotation.z = t * 0.8;

    // students
    const showFigs = ramp(t, 2.6, 3.4);
    pop(figA, showFigs); pop(figB, ramp(t, 2.9, 3.7));
    place(figA, jA, t);
    place(figB, jB, t);
    G.updateMatrixWorld(true);

    // A: the finished answer falls away; a mint draft and a small hint take its place
    const sC = c['5c'].s;
    const drop = ramp(t, sC + 1.0, sC + 2.8, ease.in);
    ansA.visible = t > 2.8 && drop < 1;
    if (ansA.visible) {
      figA.localToWorld(tmp.set(0.36, 0.95, 0.2));
      G.worldToLocal(tmp);
      ansA.position.copy(tmp);
      ansA.position.y -= drop * 2.4;
      ansA.rotation.set(-0.9 + drop * 2.0, figA.rotation.y, drop * 1.5);
      fade(ansA, (1 - drop) * ramp(t, 2.8, 3.4));
    }
    const kd = ramp(t, sC + 3.0, sC + 3.8);
    draftA.visible = kd > 0.01;
    figA.localToWorld(tmp.set(0.3, 0.95, 0.35));
    G.worldToLocal(tmp);
    draftA.position.copy(tmp);
    draftA.rotation.set(1.2, 0, 0); // held up toward the viewer
    draftA.scale.setScalar(Math.max(0.001, kd));
    const ks = ramp(t, sC + 4.4, sC + 5.6);
    spark.visible = ks > 0 && ks < 1;
    if (spark.visible) {
      orbA.getWorldPosition(tmp2); G.worldToLocal(tmp2);
      spark.position.lerpVectors(tmp2, draftA.position, ks);
      spark.rotation.y = t * 4;
    }
    setGlow(orbA, 1.2 + 1.2 * win(t, sC + 4.2, sC + 5.0, 0.3));
    pop(orbA, ramp(t, 3.0, 3.6)); pop(orbB, ramp(t, 3.3, 3.9));

    // B: convincing suggestion, checked, found wrong
    const sD = c['5d'].s, sE = c['5e'].s;
    const kSug = ramp(t, sD + 0.4, sD + 1.6, ease.back);
    const split = ramp(t, sE + 3.2, sE + 4.4, ease.out);
    const gone = ramp(t, c['5g'].s - 0.5, c['5g'].s + 0.5);
    sugg.visible = kSug > 0.001 && gone < 1;
    sugg.scale.setScalar(Math.max(0.001, 1.6 * kSug * (1 - 0.3 * split) * (1 - gone)));
    sugg.position.set(suggPos.x, suggPos.y + Math.sin(t * 1.4) * 0.04 - split * 0.25, suggPos.z);
    sugg.rotation.y = -0.4 + Math.sin(t * 0.6) * 0.08;
    sL.position.x = -0.13 - split * 0.14; sR.position.x = 0.13 + split * 0.14;
    sL.rotation.z = split * 0.35; sR.rotation.z = -split * 0.25;
    // drive the shared ornate material directly so the paper never picks up the glow
    ornate.emissiveIntensity = (0.9 + 0.6 * win(t, sD + 1.2, sE, 0.6)) * (1 - split);
    ornate.color.setHex(COLORS.amber).lerp(dimC, split);
    const kl = win(t, sE + 0.4, sE + 5.4, 0.5);
    lens.visible = kl > 0.01;
    if (lens.visible) {
      const sweep = clamp01((t - sE - 0.8) / 2.4);
      lens.position.set(suggPos.x - 0.4 + sweep * 0.8, suggPos.y + 0.3 - sweep * 0.5, suggPos.z + 0.4);
      lens.rotation.y = -0.4;
      lens.scale.setScalar(Math.max(0.001, kl));
    }

    // trajectories
    const sG = c['5g'].s, sH = c['5h'].s;
    routes.a0.userData.progress(ramp(t, sG + 0.2, sG + 1.4, ease.linear));
    routes.b0.userData.progress(ramp(t, sG + 0.5, sG + 1.7, ease.linear));
    routes.a1.userData.progress(ramp(t, sG + 1.4, sG + 4.0, ease.linear));
    routes.a2.userData.progress(ramp(t, sG + 1.6, sG + 3.6, ease.linear));
    routes.b1.userData.progress(ramp(t, sG + 1.7, sG + 4.3, ease.linear));
    routes.b2.userData.progress(ramp(t, sG + 1.9, sG + 3.9, ease.linear));
    nodes.forEach((n, i) => { pop(n, ramp(t, sG + 1.2 + i * 0.3, sG + 1.7 + i * 0.3)); setGlow(n, 1.2 + 0.8 * Math.sin(t * 3 + i)); });
    // the routes keep bending while the narration says so
    const bend = Math.sin(t * 0.9) * 0.1 * ramp(t, sH, sH + 1);
    routes.a1.rotation.y = bend * 0.25; routes.b1.rotation.y = -bend * 0.25;

    // labels
    const isl = win(t, 2.4, sB + 0.2, 0.5) + win(t, sG + 0.6, 61, 0.6);
    L.i1.set(isl); L.i2.set(isl); L.i3.set(isl);
    L.time.set(win(t, 3.0, sB + 0.2, 0.5));
    L.strong.set(win(t, sB + 0.4, sC, 0.4));
    L.q2.set(win(t, sB + 1.8, sC, 0.4));
    L.q3.set(win(t, sB + 2.4, sC, 0.4));
    L.tagA.set(win(t, 3.4, sG, 0.5));
    L.tagB.set(win(t, 3.7, sG, 0.5));
    L.stratA.set(win(t, sC + 3.2, sD + 0.4, 0.5));
    L.suggB.set(win(t, sD + 1.6, sE + 3.0, 0.4));
    L.wrongB.set(win(t, sE + 3.2, c['5f'].s + 0.6, 0.4));
    L.habit.set(win(t, c['5f'].s + 0.2, sG, 0.4));
    L.disclaimer.set(win(t, sG, 61, 0.6));
    L.ra1.set(win(t, sG + 4.0, 61, 0.5));
    L.ra2.set(win(t, sG + 3.6, 61, 0.5));
    L.rb1.set(win(t, sG + 4.3, 61, 0.5));
    L.rb2.set(win(t, sG + 3.9, 61, 0.5));
    L.design.set(win(t, sH + 2.6, c['5i'].s + 0.4, 0.4));
    const dr = win(t, sH + 2.4, c['5i'].s + 0.6, 0.5);
    designRing.visible = dr > 0.01;
    if (designRing.visible) { fade(designRing, dr); designRing.scale.setScalar(1 + 0.12 * Math.sin(t * 4)); }
    L.open.set(win(t, c['5i'].s + 0.2, 61, 0.5));
  }

  const camera = camTrack([
    { t: 0, p: [0, 8.4, 17.5], l: [0, 1.2, 0] },
    { t: c['5b'].s, p: [-1.0, 7.4, 15.4], l: [-1.0, 1.5, 0] },
    { t: c['5c'].s + 1.5, p: [-2.9, 4.7, 9.6], l: [-2.6, 1.9, -0.1] },
    { t: c['5c'].e, p: [-2.6, 4.7, 9.4], l: [-2.4, 1.9, -0.1] },
    { t: c['5d'].s + 2.0, p: [0.6, 4.6, 8.4], l: [0.3, 2.2, -0.8] },
    { t: c['5f'].e, p: [0.4, 4.7, 8.6], l: [0.3, 2.2, -0.8] },
    { t: c['5g'].s + 2.4, p: [1.0, 8.8, 15.8], l: [1.0, 1.4, 0.0] },
    { t: 60, p: [1.0, 8.4, 15.2], l: [1.0, 1.5, 0.0] },
  ]);

  return { update, camera };
}
