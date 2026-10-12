// 02 Efficiency and Independent Thinking
// The shared workspace splits into two paths. "Time saved" tokens either flow
// into further reasoning or stop at a polished answer. A pros/cons list is
// replaced by interlocking rings, and a balance of conditions steers the flow.
import {
  THREE, MAT, COLORS, at, box, cyl, mesh, glow, std, plinth, desk, chair, figure, aiPanel, card,
  tube, pop, fade, rise, ramp, win, ease, camTrack, setGlow, offset, clamp01, hash, lerp,
} from '../kit.js';

export function build(ctx) {
  const { group: G, label, overlay, cues: c } = ctx;
  G.add(plinth(8.2));

  // ---- shared workspace in two halves ----
  function half(sign) {
    const g = new THREE.Group();
    g.add(at(box(1.9, 0.14, 1.7, MAT.ivory2, 0.05), 0, 0.07, 0));
    const d = at(desk(), 0, 0.14, 0.3);
    d.scale.set(0.8, 1, 0.8);
    const ch = at(chair(), 0, 0.14, -0.3);
    const f = at(figure({ seated: true }), 0, 0.14, -0.33);
    g.add(d, ch, f);
    g.userData.figure = f;
    g.position.set(sign * 0.95, 0, 2.9);
    return g;
  }
  const hA = half(-1), hB = half(1);
  G.add(hA, hB);
  const ai = at(aiPanel(), 0, 2.35, 2.6);
  ai.scale.setScalar(0.62);
  G.add(ai);

  // ---- tracks ----
  const J = [0, 0.2, 1.4];
  const railMat = std(COLORS.ivory3, { roughness: 0.6 });
  const feed = tube([[0, 0.2, 2.6], [0, 0.2, 2.0], J], 0.04, railMat, { segs: 30 });
  const S = [[-1.9, 0.5, 0.1], [-3.15, 1.0, -1.3], [-3.85, 1.5, -2.85]];
  const END = [-3.2, 2.0, -4.3];
  const left = tube([J, [-1.0, 0.3, 0.8], S[0], S[1], S[2], END], 0.04, railMat);
  const PED = [3.75, 0.95, -1.7];
  const right = tube([J, [1.4, 0.2, 0.7], [2.8, 0.2, -0.5], [3.55, 0.45, -1.35], PED], 0.04, railMat);
  G.add(feed, left, right);

  // stations on the reasoning path
  const stations = S.map((p, i) => {
    const g = new THREE.Group();
    g.add(at(cyl(0.42, 0.46, p[1] - 0.04, MAT.ivory2, 40), 0, (p[1] - 0.04) / 2, 0));
    const top = at(cyl(0.36, 0.36, 0.03, glow(COLORS.mint, 0.2), 40), 0, p[1] - 0.025, 0);
    g.add(top);
    g.position.set(p[0], 0, p[2]);
    g.userData.top = top;
    G.add(g);
    return g;
  });
  const endPlat = new THREE.Group();
  endPlat.add(at(cyl(0.7, 0.75, END[1] - 0.05, MAT.ivory2, 48), 0, (END[1] - 0.05) / 2, 0));
  const endTop = at(cyl(0.62, 0.62, 0.04, glow(COLORS.mint, 0.25), 48), 0, END[1] - 0.03, 0);
  endPlat.add(endTop);
  endPlat.position.set(END[0], 0, END[2]);
  G.add(endPlat);

  // polished-answer pedestal and the end of the line
  const ped = new THREE.Group();
  ped.add(at(cyl(0.48, 0.52, 0.9, MAT.ivory2, 40), 0, 0.45, 0));
  const pcard = at(card({ w: 0.5, h: 0.62, accent: MAT.amberGlow }), 0, 0.98, 0);
  pcard.rotation.x = -0.5;
  ped.add(pcard);
  const pedGlow = at(cyl(0.44, 0.44, 0.02, glow(COLORS.amber, 0.3), 40), 0, 0.91, 0);
  ped.add(pedGlow);
  ped.position.set(PED[0], 0, PED[2]);
  const stopper = at(box(0.7, 0.32, 0.18, MAT.ivory3, 0.04), 4.25, 0.16, -2.25);
  stopper.rotation.y = -0.6;
  G.add(ped, stopper);

  // diverter at the junction
  const junction = at(cyl(0.5, 0.52, 0.12, MAT.ivory2, 40), J[0], 0.06, J[2]);
  const diverter = at(box(0.55, 0.28, 0.05, MAT.ivory, 0.02), J[0], 0.27, J[2]);
  G.add(junction, diverter);

  // ---- time tokens ----
  const POOL = 12;
  const tokMats = [];
  const tokens = [];
  for (let i = 0; i < POOL; i++) {
    const m = glow(COLORS.amber, 1.8);
    tokMats.push(m);
    const tk = mesh(new THREE.CylinderGeometry(0.13, 0.13, 0.05, 28), m, { cast: false });
    G.add(tk);
    tokens.push(tk);
  }
  const amberC = new THREE.Color(COLORS.amber), mintC = new THREE.Color(COLORS.mint);
  const EMIT0 = c['2c'].s - 1.2, EVERY = 0.9, FEED = 1.0, LEFT_T = 4.2, RIGHT_T = 2.8, END_T = 63.4;
  const US = [0.3, 0.6, 0.85]; // station positions along the left path (arc-length fraction)
  function pLeft(tj) {
    if (tj < c['2e'].s) return 1;
    if (tj < c['2f'].s) return 0;
    if (tj < c['2k'].s + 1.2) return 0.5;
    if (tj < c['2l'].s + 0.8) return 0.92;
    return 0.1;
  }
  const routeOf = (k) => (hash(k + 0.5) < pLeft(EMIT0 + k * EVERY + FEED) ? 'L' : 'R');

  // ---- pros / cons columns, then interlocking rings ----
  const listG = at(new THREE.Group(), 0, 3.25, -2.4);
  const slabL = at(box(1.5, 1.6, 0.08, MAT.ivory, 0.05), -0.85, 0, 0);
  const slabR = at(box(1.5, 1.6, 0.08, MAT.ivory, 0.05), 0.85, 0, 0);
  for (const s of [slabL, slabR]) {
    s.add(at(box(1.2, 0.12, 0.02, MAT.ivory3, 0.02), 0, 0.6, 0.05));
    for (let i = 0; i < 3; i++) s.add(at(box(1.0 - i * 0.18, 0.05, 0.02, MAT.ivory2, 0.01), -0.1 + i * 0.09 - 0.0, 0.25 - i * 0.28, 0.05));
  }
  const strike = at(box(3.6, 0.07, 0.04, MAT.amberGlow, 0.02), 0, 0, 0.12);
  strike.rotation.z = 0.18;
  listG.add(slabL, slabR, strike);
  G.add(listG);

  const ringsG = at(new THREE.Group(), 0, 3.3, -2.4);
  const R = 0.62;
  const ringE = at(mesh(new THREE.TorusGeometry(R, 0.06, 16, 96), glow(COLORS.amber, 1.0)), -0.4, 0, 0);
  const ringT = at(mesh(new THREE.TorusGeometry(R, 0.06, 16, 96), glow(COLORS.mint, 0.9)), 0.4, 0, 0);
  ringT.rotation.x = Math.PI / 2;
  const runE = at(mesh(new THREE.SphereGeometry(0.075, 16, 12), MAT.amberGlow), 0, 0, 0);
  const runT = at(mesh(new THREE.SphereGeometry(0.075, 16, 12), MAT.mintGlow), 0, 0, 0);
  ringsG.add(ringE, ringT, runE, runT);
  G.add(ringsG);

  // ---- balance of conditions ----
  const bal = at(new THREE.Group(), J[0], 0, J[2] + 0.1);
  const post = at(cyl(0.035, 0.05, 1.25, MAT.ivory3, 16), 0, 0.62, 0);
  const beam = at(new THREE.Group(), 0, 1.3, 0);
  beam.add(at(box(3.1, 0.07, 0.09, MAT.ivory, 0.02), 0, 0, 0));
  const pans = [-1.45, 1.45].map((x) => {
    const p = at(new THREE.Group(), x, 0, 0);
    p.add(at(cyl(0.006, 0.006, 0.36, MAT.ivory3, 6), 0, -0.18, 0));
    p.add(at(cyl(0.44, 0.4, 0.05, MAT.ivory2, 32), 0, -0.38, 0));
    beam.add(p);
    return p;
  });
  bal.add(post, beam);
  G.add(bal);
  function chip(accent) {
    const g = new THREE.Group();
    g.add(at(box(0.56, 0.15, 0.36, MAT.ivory, 0.03), 0, 0.075, 0));
    g.add(at(box(0.56, 0.035, 0.07, accent, 0.01), 0, 0.15, -0.12));
    return g;
  }
  const chips = [
    { m: chip(MAT.mintGlow), pan: 0, slot: 0, inT: c['2k'].s + 0.2, outT: c['2l'].s + 0.2, w: 1 },
    { m: chip(MAT.mintGlow), pan: 0, slot: 1, inT: c['2k'].s + 2.4, outT: c['2l'].s + 0.5, w: 1 },
    { m: chip(MAT.amberGlow), pan: 1, slot: 0, inT: c['2l'].s + 0.6, outT: 99, w: 1.3 },
    { m: chip(MAT.amberGlow), pan: 1, slot: 1, inT: c['2l'].s + 3.0, outT: 99, w: 0.9 },
  ];
  chips.forEach((ch) => pans[ch.pan].add(ch.m));

  // ---- labels ----
  const L = {
    habits: label({ html: 'Habits in progress, <b>not fixed traits</b>', anchor: [0, 1.95, 2.9], cls: 'tag ivory', zh: '正在形成的习惯，而非固定性格' }),
    leftPath: label({ html: '<b>Time saved → more reasoning</b>', anchor: [-1.4, 0.55, 0.55], dy: 4, align: 'below', cls: 'tag mint' }),
    st: [
      label({ html: 'Test an example', anchor: [S[0][0], S[0][1] + 0.12, S[0][2]], dx: -14, align: 'right', cls: 'tag small' }),
      label({ html: 'Compare explanations', anchor: [S[1][0], S[1][1] + 0.12, S[1][2]], dx: -14, align: 'right', cls: 'tag small' }),
      label({ html: 'Explain in your own words', anchor: [S[2][0], S[2][1] + 0.12, S[2][2]], dx: -14, align: 'right', cls: 'tag small' }),
    ],
    endL: label({ html: 'Ready to explain, test, judge', anchor: [END[0], END[1] + 0.1, END[2]], cls: 'tag mint small' }),
    rightPath: label({ html: '<b>Process stops at a polished answer</b>', anchor: [PED[0], PED[1] + 0.35, PED[2]], cls: 'tag amber' }),
    vanish: label({ html: 'saved time disappears', anchor: [4.25, 0.35, -2.25], dx: 10, align: 'left', cls: 'tag small' }),
    pros: label({ html: '<b>Pros</b><br>saves time<br>quick feedback', anchor: slabL, cls: 'list-card', align: 'center' }),
    cons: label({ html: '<b>Cons</b><br>less effort<br>shallow thinking', anchor: slabR, cls: 'list-card', align: 'center' }),
    notCols: label({ html: 'Not two separate columns', anchor: [0, 2.25, -2.4], align: 'below', cls: 'tag amber' }),
    eff: label({ html: '<b>Efficiency</b>', anchor: ringE, dx: -70, align: 'right', cls: 'tag amber', zh: '效率' }),
    ind: label({ html: '<b>Independent thinking</b>', anchor: ringT, dx: 70, align: 'left', cls: 'tag mint', zh: '独立思考' }),
    act1: label({ html: 'saved time can feed reasoning', anchor: [0, 4.2, -2.4], cls: 'tag small' }),
    act2: label({ html: 'reasoning lets you judge the output', anchor: [0, 2.45, -2.4], align: 'below', cls: 'tag small' }),
    cond: label({ html: '<b>Conditions</b> decide which way the influence runs', anchor: [0, 1.45, J[2] + 0.1], dy: -6, cls: 'tag ivory', zh: '条件决定影响的方向' }),
    chip0: label({ html: 'Task demands explanation', anchor: chips[0].m, dx: -46, dy: 6, align: 'right', cls: 'tag small mint' }),
    chip1: label({ html: 'Teacher checks the process', anchor: chips[1].m, dx: -46, dy: -26, align: 'right', cls: 'tag small mint' }),
    chip2: label({ html: 'Only the final product is graded', anchor: chips[2].m, dx: 46, dy: 6, align: 'left', cls: 'tag small amber' }),
    chip3: label({ html: 'Tight deadline', anchor: chips[3].m, dx: 46, dy: -26, align: 'left', cls: 'tag small amber' }),
  };

  const tmp = new THREE.Vector3();

  function update(lt) {
    const t = lt;
    // workspace splits
    const ks = ramp(t, c['2b'].s + 0.2, c['2b'].s + 2.6);
    offset(hA, -2.35 * ks, 0, -0.7 * ks);
    offset(hB, 2.35 * ks, 0, -0.7 * ks);
    hA.rotation.y = 0.55 * ks;
    hB.rotation.y = -0.55 * ks;
    ai.userData.animate(t);
    // after the split the shared AI settles low between the students, where the time tokens start
    ai.position.set(0, lerp(2.35, 0.75, ks) + Math.sin(t * 0.9) * 0.04, lerp(2.6, 3.2, ks));
    ai.scale.setScalar(lerp(0.62, 0.42, ks));
    ai.rotation.x = -0.9 * ks;
    feed.userData.progress(ramp(t, c['2b'].s + 0.8, c['2b'].s + 1.8));
    left.userData.progress(ramp(t, c['2b'].s + 1.6, c['2b'].s + 3.8));
    right.userData.progress(ramp(t, c['2b'].s + 1.9, c['2b'].s + 3.8));
    stations.forEach((s, i) => pop(s, ramp(t, c['2b'].s + 2.0 + i * 0.35, c['2b'].s + 2.8 + i * 0.35)));
    pop(endPlat, ramp(t, c['2b'].s + 3.0, c['2b'].s + 3.8));
    pop(ped, ramp(t, c['2b'].s + 2.6, c['2b'].s + 3.4));
    pop(stopper, ramp(t, c['2b'].s + 3.0, c['2b'].s + 3.6));
    pop(junction, ramp(t, c['2b'].s + 1.2, c['2b'].s + 1.8));
    pop(diverter, ramp(t, c['2b'].s + 1.4, c['2b'].s + 2.0));

    // tokens (each one is a pure function of its emission time)
    tokens.forEach((tk) => (tk.visible = false));
    const glowS = [0, 0, 0];
    let glowEnd = 0, glowPed = 0;
    const kMax = Math.floor((Math.min(t, END_T) - EMIT0) / EVERY);
    for (let k = Math.max(0, kMax - POOL + 1); k <= kMax; k++) {
      const e = EMIT0 + k * EVERY;
      const age = t - e;
      if (age < 0) continue;
      const route = routeOf(k);
      const tk = tokens[k % POOL];
      const mat = tokMats[k % POOL];
      const life = FEED + (route === 'L' ? LEFT_T + 0.6 : RIGHT_T + 0.9);
      if (age > life) continue;
      tk.visible = true;
      tk.scale.setScalar(Math.min(1, age / 0.25));
      if (age < FEED) {
        feed.userData.curve.getPointAt(age / FEED, tk.position);
        mat.color.copy(amberC); mat.emissive.copy(amberC);
      } else if (route === 'L') {
        const u = clamp01((age - FEED) / LEFT_T);
        left.userData.curve.getPointAt(u, tk.position);
        const m = ramp(u, 0.2, 0.75);
        mat.color.copy(amberC).lerp(mintC, m);
        mat.emissive.copy(mat.color);
        US.forEach((us, i) => {
          const dt = age - FEED - us * LEFT_T;
          if (dt > 0) glowS[i] = Math.max(glowS[i], Math.exp(-dt / 0.9));
        });
        if (u >= 1) { const dt = age - FEED - LEFT_T; glowEnd = Math.max(glowEnd, Math.exp(-dt / 1.2)); tk.scale.setScalar(Math.max(0.001, 1 - dt / 0.6)); }
      } else {
        const u = clamp01((age - FEED) / RIGHT_T);
        right.userData.curve.getPointAt(u, tk.position);
        mat.color.copy(amberC); mat.emissive.copy(amberC);
        if (u >= 1) {
          const dt = age - FEED - RIGHT_T;
          glowPed = Math.max(glowPed, Math.exp(-dt / 0.7));
          tk.scale.setScalar(Math.max(0.001, 1 - dt / 0.9));
          tk.position.y += dt * 0.25;
        }
      }
      tk.position.y += 0.07;
    }
    stations.forEach((s, i) => setGlow(s.userData.top, 0.25 + 1.6 * glowS[i]));
    setGlow(endTop, 0.3 + 1.4 * glowEnd);
    setGlow(pedGlow, 0.3 + 1.5 * glowPed);

    // balance
    const sJ = c['2j'].s;
    rise(bal, ramp(t, sJ - 0.4, sJ + 1.0), 1.6);
    let wL = 0, wR = 0;
    chips.forEach((ch) => {
      const kin = ramp(t, ch.inT, ch.inT + 0.8, ease.out);
      const kout = ramp(t, ch.outT, ch.outT + 0.8);
      const k = kin * (1 - kout);
      ch.m.visible = kin > 0 && kout < 1;
      ch.m.position.set((ch.slot - 0.5) * 0.12, -0.35 + ch.slot * 0.15 + (1 - kin) * 1.6 + kout * 1.4, (ch.slot - 0.5) * 0.06);
      if (ch.m.visible) fade(ch.m, kin * (1 - kout));
      if (ch.pan === 0) wL += k * ch.w; else wR += k * ch.w;
    });
    const tilt = THREE.MathUtils.clamp((wR - wL) * 0.16, -0.32, 0.32);
    beam.rotation.z = -tilt;
    pans.forEach((p) => (p.rotation.z = tilt));
    // the diverter leans toward the path it is closing
    const pl = pLeft(t);
    diverter.rotation.y = (pl - 0.5) * 1.6;

    // pros/cons → rings
    const sF = c['2f'].s, sG = c['2g'].s, sH = c['2h'].s;
    const kList = ramp(t, sF, sF + 1.0) * (1 - ramp(t, sG + 2.0, sH + 0.2));
    listG.visible = kList > 0.001;
    if (listG.visible) {
      pop(slabL, ramp(t, sF, sF + 0.8));
      pop(slabR, ramp(t, sF + 0.2, sF + 1.0));
      fade(listG, kList);
      slabL.rotation.z = 0.25 * ramp(t, sG + 1.0, sH);
      slabR.rotation.z = -0.25 * ramp(t, sG + 1.0, sH);
      offset(slabL, -0.5 * ramp(t, sG + 1.0, sH), 0, 0);
      offset(slabR, 0.5 * ramp(t, sG + 1.0, sH), 0, 0);
    }
    strike.visible = t > sG + 0.6 && listG.visible;
    strike.scale.x = Math.max(0.001, ramp(t, sG + 0.6, sG + 1.2));

    const kR = ramp(t, sH - 0.2, sH + 1.2, ease.out);
    const kUp = ramp(t, sJ, sJ + 1.6);
    ringsG.visible = kR > 0.001;
    ringsG.scale.setScalar(Math.max(0.001, kR * (1 - 0.35 * kUp)));
    ringsG.position.y = 3.3 + kUp * 0.35;
    ringsG.rotation.y = 0.35 + Math.sin(t * 0.35) * 0.25;
    ringsG.rotation.x = 0.15;
    const a1 = t * 1.6, a2 = -t * 1.6 + 1.2;
    runE.position.set(-0.4 + Math.cos(a1) * R, Math.sin(a1) * R, 0);
    runT.position.set(0.4 + Math.cos(a2) * R, 0, Math.sin(a2) * R);

    // labels
    L.habits.set(win(t, c['2a'].s - 0.2, c['2b'].s + 1.5, 0.5));
    const sC = c['2c'].s, sE = c['2e'].s;
    L.leftPath.set(win(t, sC, sE, 0.5));
    L.st.forEach((l, i) => l.set(win(t, c['2d'].s + i * 1.4, sE + 0.5, 0.4)));
    L.endL.set(win(t, c['2d'].s + 3.8, sE + 0.5, 0.4));
    L.rightPath.set(win(t, sE + 0.4, sF, 0.5));
    L.vanish.set(win(t, sE + 3.0, sF, 0.5));
    L.pros.set(win(t, sF + 0.6, sG + 2.0, 0.5));
    L.cons.set(win(t, sF + 0.8, sG + 2.0, 0.5));
    L.notCols.set(win(t, sG + 1.0, sH + 0.4, 0.4));
    L.eff.set(win(t, sH + 0.6, sJ + 0.6, 0.5));
    L.ind.set(win(t, sH + 0.8, sJ + 0.6, 0.5));
    L.act1.set(win(t, c['2i'].s, sJ, 0.5));
    L.act2.set(win(t, c['2i'].s + 2.6, sJ, 0.5));
    L.cond.set(win(t, sJ + 0.6, c['2k'].s + 0.4, 0.5));
    const chipVis = (i) => {
      const ch = chips[i];
      return win(t, ch.inT + 0.6, Math.min(ch.outT, 65.5), 0.4);
    };
    L.chip0.set(chipVis(0)); L.chip1.set(chipVis(1)); L.chip2.set(chipVis(2)); L.chip3.set(chipVis(3));
  }

  const camera = camTrack([
    { t: 0, p: [0, 7.2, 14.2], l: [0, 0.9, 1.2] },
    { t: c['2b'].e, p: [0, 8.2, 14.4], l: [0, 0.8, -0.5] },
    { t: c['2c'].s + 1.2, p: [-1.8, 6.6, 11.6], l: [-2.1, 1.1, -1.2] },
    { t: c['2d'].e, p: [-1.6, 6.6, 11.6], l: [-2.1, 1.2, -1.4] },
    { t: c['2e'].s + 1.2, p: [1.9, 6.2, 11.4], l: [2.3, 0.9, -0.6] },
    { t: c['2e'].e, p: [1.8, 6.2, 11.4], l: [2.2, 0.9, -0.6] },
    { t: c['2f'].s + 1.0, p: [0, 6.6, 11.6], l: [0, 2.5, -1.6] },
    { t: c['2i'].e, p: [0, 6.8, 11.4], l: [0, 2.7, -1.8] },
    { t: c['2j'].s + 1.6, p: [0, 5.6, 10.6], l: [0, 1.5, 0.3] },
    { t: c['2l'].s, p: [0, 5.5, 10.2], l: [0, 1.4, 0.4] },
    { t: 65, p: [0, 6.2, 11.6], l: [0, 1.3, 0.0] },
  ]);

  return { update, camera };
}
