// 07 Ask a Better Question
// The two workspaces reconnect. The smarter/more-dependent seesaw breaks and
// four questions rise in its place. The film ends on the learner making a
// decision and then evaluating it.
import {
  THREE, MAT, COLORS, at, box, cyl, mesh, glow, std, plinth, studySpace,
  tube, pop, fade, rise, ramp, win, ease, camTrack, setGlow, offset, clamp01,
} from '../kit.js';

export function build(ctx) {
  const { group: G, label, overlay, cues: c } = ctx;
  G.add(plinth(8.4));

  const A = at(studySpace({ seed: 6, who: 'A' }), -3.9, 0, 0.6);
  const B = at(studySpace({ seed: 7, who: 'B' }), 3.9, 0, 0.6);
  A.rotation.y = 0.2; B.rotation.y = -0.2;
  G.add(A, B);
  const seam = at(box(1.2, 0.14, 2.2, MAT.ivory2, 0.05), 0, 0.07, 0.75);
  const seamGlow = at(box(0.12, 0.02, 2.0, MAT.mintGlow, 0.01), 0, 0.15, 0.75);
  G.add(seam, seamGlow);

  // the either/or seesaw
  const see = at(new THREE.Group(), 0, 2.7, -0.6);
  const fulcrum = mesh(new THREE.ConeGeometry(0.22, 0.4, 3), MAT.ivory3);
  fulcrum.position.y = -0.2;
  const beam = at(new THREE.Group(), 0, 0.04, 0);
  const halfL = at(new THREE.Group(), 0, 0, 0), halfR = at(new THREE.Group(), 0, 0, 0);
  const plaqueL = at(box(1.0, 0.46, 0.06, MAT.ivory, 0.04), -1.5, 0.3, 0);
  const plaqueR = at(box(1.0, 0.46, 0.06, MAT.ivory, 0.04), 1.5, 0.3, 0);
  halfL.add(at(box(1.7, 0.07, 0.12, MAT.ivory, 0.02), -0.85, 0, 0), plaqueL);
  halfR.add(at(box(1.7, 0.07, 0.12, MAT.ivory, 0.02), 0.85, 0, 0), plaqueR);
  beam.add(halfL, halfR);
  see.add(fulcrum, beam);
  G.add(see);

  // four questions
  const PX = [-3.7, -1.35, 1.35, 3.7], PH = [1.7, 2.05, 2.05, 1.7];
  const pillars = PX.map((x, i) => {
    const g = at(new THREE.Group(), x, 0, -2.7 + (i === 0 || i === 3 ? 0.5 : 0));
    g.add(at(box(0.72, PH[i], 0.72, MAT.ivory, 0.05), 0, PH[i] / 2, 0));
    g.add(at(box(0.8, 0.06, 0.8, MAT.ivory2, 0.02), 0, PH[i] + 0.03, 0));
    const em = at(new THREE.Group(), 0, PH[i] + 0.06, 0);
    if (i === 0) { // conditions: switches
      for (let k = 0; k < 2; k++) {
        const base = at(box(0.22, 0.08, 0.3, MAT.ivory2, 0.02), (k - 0.5) * 0.32, 0.04, 0);
        const lever = at(cyl(0.018, 0.018, 0.28, MAT.ivory3, 8), 0, 0.16, 0);
        lever.rotation.z = k ? -0.5 : 0.5;
        lever.add(at(mesh(new THREE.SphereGeometry(0.05, 12, 10), k ? MAT.amberGlow : MAT.mintGlow), 0, 0.15, 0));
        base.add(lever);
        em.add(base);
      }
    } else if (i === 1) { // central difficulty: one block among many
      for (let gx = -1; gx <= 1; gx++) for (let gz = -1; gz <= 1; gz++) {
        const center = gx === 0 && gz === 0;
        em.add(at(box(0.15, center ? 0.22 : 0.1, 0.15, center ? MAT.amberGlow : MAT.ivory3, 0.02), gx * 0.19, center ? 0.11 : 0.05, gz * 0.19));
      }
    } else if (i === 2) { // time: three linked islets
      for (let k = 0; k < 3; k++) em.add(at(cyl(0.09, 0.07, 0.07, MAT.ivory2, 16), (k - 1) * 0.24, 0.04 + k * 0.05, 0));
      em.add(at(box(0.5, 0.02, 0.04, MAT.mintSoft, 0.005), 0, 0.1, 0));
    } else { // test: a small cycle
      const r = mesh(new THREE.TorusGeometry(0.2, 0.025, 10, 48), MAT.mintGlow);
      r.rotation.x = Math.PI / 2;
      r.position.y = 0.06;
      const dot = at(mesh(new THREE.SphereGeometry(0.05, 12, 10), MAT.mintGlow), 0.2, 0.06, 0);
      em.add(r, dot);
      em.userData.dot = dot;
    }
    g.add(em);
    g.userData = { em, top: PH[i] + 0.3 };
    G.add(g);
    return g;
  });

  // a grade that can't answer them
  const medal = new THREE.Group();
  medal.add(mesh(new THREE.CylinderGeometry(0.28, 0.28, 0.05, 40), MAT.amberGlow));
  const mr = mesh(new THREE.TorusGeometry(0.34, 0.025, 8, 48), MAT.amberGlow);
  mr.rotation.x = Math.PI / 2;
  medal.add(mr);
  medal.position.set(0, 2.2, 0.9);
  G.add(medal);

  // the learner's decision, then its evaluation
  const ped = at(cyl(0.32, 0.36, 0.5, MAT.ivory2, 32), 0, 0.14 + 0.25, 1.2);
  const decision = at(box(0.36, 0.3, 0.36, MAT.mintGlow, 0.05), 0, 0.14 + 0.5 + 0.15, 1.2);
  const scan = at(mesh(new THREE.TorusGeometry(0.42, 0.02, 8, 64), MAT.amberGlow, { cast: false }), 0, 0.7, 1.2);
  scan.rotation.x = Math.PI / 2;
  G.add(ped, decision, scan);
  const beams = pillars.map((p, i) => {
    const s = p.position.clone(); s.y = p.userData.top;
    const e = new THREE.Vector3(0, 0.95, 1.2);
    const mid = s.clone().lerp(e, 0.5); mid.y += 0.8;
    const tb = tube([[s.x, s.y, s.z], [mid.x, mid.y, mid.z], [e.x, e.y, e.z]], 0.012, glow(i === 1 ? COLORS.amber : COLORS.mint, 1.2), { segs: 60 });
    G.add(tb);
    return tb;
  });

  // ---------- labels ----------
  const qs = ['Under what<br>conditions?', 'What is the central<br>difficulty?', 'What changes<br>over time?', 'How can we test<br>our judgment?'];
  const qzh = ['在什么条件下？', '核心难点是什么？', '随着时间推移，什么在变化？', '我们如何检验自己的判断？'];
  const qCue = ['7c', '7d', '7e', '7f'];
  const L = {
    smarter: label({ html: '<b>Smarter</b>', anchor: plaqueL, dy: -20, cls: 'tag ivory big', zh: '更聪明' }),
    dependent: label({ html: '<b>More dependent</b>', anchor: plaqueR, dy: -20, cls: 'tag ivory big', zh: '更依赖' }),
    either: label({ html: 'either / or?', anchor: [0, 2.5, -0.6], align: 'below', dy: 8, cls: 'tag small' }),
    q: pillars.map((p, i) => label({ html: qs[i], anchor: [p.position.x, p.userData.top + 0.15, p.position.z], cls: 'question-tag', zh: qzh[i] })),
    grade: label({ html: 'A higher grade, alone', anchor: medal, dy: -26, cls: 'tag amber small', zh: '单凭一次更高的成绩' }),
    decide: label({ html: '<b>Decide</b> how to use the tool', anchor: [0, 0.2, 1.75], dx: -30, align: 'right', cls: 'tag mint', zh: '决定如何使用工具' }),
    evaluate: label({ html: '<b>Check</b> whether the decision worked', anchor: [0, 0.2, 1.75], dx: 30, align: 'left', cls: 'tag amber', zh: '检验这个决定是否有效' }),
    tagA: label({ html: '<b>A</b>', anchor: [-1.9, 1.9, 0.6], cls: 'tag ivory small' }),
    tagB: label({ html: '<b>B</b>', anchor: [1.9, 1.9, 0.6], cls: 'tag ivory small' }),
    end: overlay(`<div class="endcard"><h1>Does AI Make Us Smarter&#8288;<span>—</span>or More Dependent?</h1><p class="tag-line">Ask better questions. Decide. Test. Revise.</p><p class="fine">A dramatization. The students are fictional; no performance data is shown.</p></div>`, 'center-mid', 'AI 让我们更聪明，还是更依赖？提出更好的问题。做出决定。检验。修正。'),
  };

  function update(lt) {
    const t = lt;
    // reconnect
    const kj = ramp(t, c['7a'].s + 0.4, c['7a'].s + 3.6);
    offset(A, 2.0 * kj, 0, 0);
    offset(B, -2.0 * kj, 0, 0);
    rise(seam, ramp(t, c['7a'].s + 2.4, c['7a'].s + 3.8), 0.4);
    pop(seamGlow, ramp(t, c['7a'].s + 3.4, c['7a'].s + 4.2));
    setGlow(seamGlow, 0.25 + 0.55 * win(t, c['7a'].s + 3.4, c['7a'].s + 6, 0.6));

    // seesaw rocks, then breaks
    const sB = c['7b'].s;
    const ks = ramp(t, sB - 0.4, sB + 0.8);
    const brk = ramp(t, sB + 3.4, sB + 5.0, ease.in);
    see.visible = ks > 0.001 && brk < 1;
    see.scale.setScalar(Math.max(0.001, ease.back(ks)));
    beam.rotation.z = Math.sin(t * 1.6) * 0.16 * (1 - brk);
    halfL.position.set(-brk * 0.6, -brk * 2.4, 0); halfL.rotation.z = brk * 0.9;
    halfR.position.set(brk * 0.6, -brk * 2.4, 0); halfR.rotation.z = -brk * 0.9;
    fulcrum.position.y = -0.2 - brk * 2.0;
    if (see.visible) fade(see, 1 - brk);

    // pillars rise with their questions
    pillars.forEach((p, i) => {
      const s = c[qCue[i]].s;
      rise(p, ramp(t, s - 0.6, s + 0.8), 2.4);
      if (p.userData.em.userData.dot) {
        const a = t * 2;
        p.userData.em.userData.dot.position.set(Math.cos(a) * 0.2, 0.06, Math.sin(a) * 0.2);
      }
    });
    pillars[1].userData.em.rotation.y = t * 0.3;

    // the grade
    const sG = c['7g'].s;
    const km = win(t, sG - 0.2, sG + 3.6, 0.5);
    pop(medal, km);
    medal.rotation.y = t * 1.2;
    medal.position.y = 2.2 - ramp(t, sG + 2.2, sG + 3.6) * 0.8;

    // decide, then evaluate
    const sH = c['7h'].s;
    pop(ped, ramp(t, sH - 1.0, sH));
    const kd = ramp(t, sH + 0.4, sH + 1.6, ease.out);
    decision.visible = kd > 0.001;
    decision.position.y = 0.14 + 0.5 + 0.15 + (1 - kd) * 1.2;
    decision.rotation.y = (1 - kd) * 1.5;
    beams.forEach((b, i) => b.userData.progress(ramp(t, sH + 1.6 + i * 0.25, sH + 2.8 + i * 0.25, ease.linear)));
    const sc = clamp01((t - sH - 3.6) / 1.6);
    scan.visible = sc > 0 && sc < 1;
    scan.position.y = 0.6 + Math.sin(sc * Math.PI) * 0.6;
    const pass = ramp(t, sH + 5.0, sH + 5.6);
    setGlow(decision, 1.2 + 1.4 * pass * (0.6 + 0.4 * Math.sin(t * 3)));
    // students look at the decision
    const look = ramp(t, sH, sH + 1.2);
    A.userData.figure.userData.head.rotation.y = look * 0.5;
    B.userData.figure.userData.head.rotation.y = -look * 0.5;
    A.userData.figure.userData.head.rotation.x = look * 0.18;
    B.userData.figure.userData.head.rotation.x = look * 0.18;

    // labels
    L.smarter.set(win(t, sB + 0.4, sB + 3.6, 0.4));
    L.dependent.set(win(t, sB + 0.6, sB + 3.6, 0.4));
    L.either.set(win(t, sB + 1.0, sB + 3.4, 0.4));
    L.q.forEach((l, i) => l.set(win(t, c[qCue[i]].s + 0.2, c['7h'].e + 0.2, 0.5)));
    L.grade.set(win(t, sG + 0.2, sG + 3.2, 0.4));
    L.decide.set(win(t, sH + 1.0, c['7h'].e + 0.2, 0.4));
    L.evaluate.set(win(t, sH + 4.0, c['7h'].e + 0.2, 0.4));
    L.tagA.set(win(t, c['7a'].s + 1.0, sB, 0.5));
    L.tagB.set(win(t, c['7a'].s + 1.0, sB, 0.5));
    L.end.set(ramp(t, c['7h'].e + 0.3, c['7h'].e + 1.3));
  }

  const camera = camTrack([
    { t: 0, p: [0, 6.4, 13.6], l: [0, 1.3, 0] },
    { t: c['7a'].e, p: [0, 6.2, 13.0], l: [0, 1.6, -0.2] },
    { t: c['7b'].s + 1.0, p: [0, 6.2, 12.2], l: [0, 2.3, -0.6] },
    { t: c['7c'].s, p: [0, 6.6, 12.6], l: [0, 2.0, -1.2] },
    { t: c['7f'].e, p: [0, 6.6, 12.2], l: [0, 2.0, -1.3] },
    { t: c['7g'].s + 1.0, p: [0, 5.8, 11.4], l: [0, 1.7, -0.2] },
    { t: c['7h'].s + 1.6, p: [0, 4.6, 9.4], l: [0, 1.2, 0.4] },
    { t: c['7h'].e, p: [0, 4.8, 9.6], l: [0, 1.3, 0.2] },
    { t: 40, p: [0, 7.0, 16.4], l: [0, 4.6, -0.6] }, // tilt up: the scene settles under the end card
  ]);

  return { update, camera };
}
