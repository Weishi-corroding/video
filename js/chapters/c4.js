// 04 Find the Decisive Problem
// Two construction models of reasoning carry many small flaws. A load test
// (the teacher's follow-up) shows where each one actually fails: A at the
// correlation→causation joint, B through thin evidence and unanswered rivals.
// Fixing the decisive problem changes how the minor ones look.
import {
  THREE, MAT, COLORS, at, box, cyl, mesh, glow, std, plinth, arrow,
  pop, fade, rise, ramp, win, ease, camTrack, setGlow, offset, clamp01, hash,
} from '../kit.js';

export function build(ctx) {
  const { group: G, label, overlay, cues: c } = ctx;
  G.add(plinth(8.4));
  const bench = at(box(10.2, 0.24, 4.2, MAT.ivory3, 0.06), 0, 0.12, 0);
  G.add(bench);
  const Y0 = 0.32;

  function pin(seed) {
    const g = new THREE.Group();
    g.add(at(cyl(0.008, 0.008, 0.16, MAT.ivory3, 6), 0, 0.08, 0));
    const head = at(mesh(new THREE.SphereGeometry(0.045, 12, 10), glow(COLORS.amber, 0.7)), 0, 0.17, 0);
    g.add(head);
    g.rotation.x = Math.PI / 2 + (hash(seed + 9) - 0.5) * 0.5; // point out of the face, toward the viewer
    g.rotation.z = (hash(seed) - 0.5) * 0.6;
    g.userData.head = head;
    return g;
  }

  // ---------- A: tall, polished, but the joint is weak ----------
  const A = at(new THREE.Group(), -2.7, Y0, 0);
  A.add(at(box(2.8, 0.08, 2.0, MAT.ivory2, 0.03), 0, -0.04, 0));
  const fA = at(box(1.7, 0.5, 1.1, MAT.ivory, 0.04), 0, 0.25, 0);
  const jL = at(box(0.84, 0.14, 0.9, MAT.ivory2, 0.03), -0.42, 0.57, 0);
  const jR = at(box(0.84, 0.14, 0.9, MAT.ivory2, 0.03), 0.42, 0.57, 0);
  const crack = at(box(0.09, 0.2, 0.96, glow(COLORS.amber, 2.6), 0.01), 0, 0.57, 0);
  const brace = at(box(1.72, 0.15, 0.96, MAT.mintSoft, 0.035), 0, 0.57, 0);
  A.add(fA, jL, jR, crack, brace);
  const U = at(new THREE.Group(), 0.85, 0.64, 0); // pivot at the joint's right edge
  const u1 = at(box(1.5, 0.45, 1.0, MAT.ivory, 0.04), -0.85, 0.225, 0);
  const u2 = at(box(1.2, 0.4, 0.86, MAT.ivory, 0.04), -0.85, 0.65, 0);
  const u3 = at(box(0.9, 0.32, 0.7, std(COLORS.amber, { roughness: 0.4, metalness: 0.15, emissive: COLORS.amber, emissiveIntensity: 0.12 }), 0.05), -0.85, 1.01, 0);
  const wA = at(new THREE.Group(), -0.85, 1.17, 0);
  U.add(u1, u2, u3, wA);
  A.add(U);
  // minor flaws: pins sticking out of block faces, [parent, x, y, z] in the block's frame
  const placePins = (list, seed0) => list.map(([parent, x, y, z], i) => {
    const p = pin(seed0 + i);
    p.position.set(x, y, z);
    parent.add(p);
    return p;
  });
  const pinsA = placePins([[fA, -0.5, 0.05, 0.55], [u1, 0.45, 0.05, 0.5], [u2, -0.35, 0.05, 0.43], [u2, 0.3, -0.08, 0.43], [u3, 0.15, 0.0, 0.35], [fA, 0.55, -0.1, 0.55]], 1);
  // three of A's minor flaws stop mattering once the joint is rebuilt
  const fadesWithFix = [1, 2, 4];

  // ---------- B: stands, but sways ----------
  const B = at(new THREE.Group(), 2.7, Y0, 0);
  B.add(at(box(2.8, 0.08, 2.0, MAT.ivory2, 0.03), 0, -0.04, 0));
  const slab = at(box(2.4, 0.3, 1.2, MAT.ivory, 0.04), 0, 0.15, 0);
  B.add(slab);
  const S = at(new THREE.Group(), 0, 0.3, 0); // sway pivot
  const pillars = [-0.7, 0.7].map((x) => at(cyl(0.055, 0.065, 1.0, MAT.ivory, 16), x, 0.5, 0));
  const sleeves = [-0.7, 0.7].map((x) => at(cyl(0.15, 0.16, 1.0, MAT.mintSoft, 24), x, 0.5, 0));
  const deck = at(box(2.4, 0.2, 1.1, MAT.ivory, 0.04), 0, 1.1, 0);
  const deckTop = at(box(0.9, 0.22, 0.6, MAT.ivory2, 0.04), 0.2, 1.31, 0);
  const wB = at(new THREE.Group(), -0.35, 1.2, 0);
  S.add(...pillars, ...sleeves, deck, deckTop, wB);
  B.add(S);
  const ghostMat = new THREE.LineBasicMaterial({ color: COLORS.amber, transparent: true, opacity: 0.9 });
  const slots = [-1, 1].map((sg) => {
    const g = new THREE.LineSegments(new THREE.EdgesGeometry(new THREE.BoxGeometry(0.3, 0.95, 0.5)), ghostMat);
    g.position.set(sg * 1.38, 0.78, 0);
    g.rotation.z = -sg * 0.32;
    B.add(g);
    return g;
  });
  const buttresses = [-1, 1].map((sg) => {
    const b = at(box(0.3, 0.95, 0.5, MAT.mintSoft, 0.04), sg * 1.38, 0.78, 0);
    b.rotation.z = -sg * 0.32;
    B.add(b);
    return b;
  });
  const pinsB = placePins([[slab, -0.8, 0.0, 0.6], [slab, 0.75, 0.04, 0.6], [deck, -0.6, 0, 0.55], [deck, 0.85, 0, 0.55], [deckTop, -0.2, 0.0, 0.3]], 20);
  const alts = [-1, 1].map((sg) => {
    const a = arrow(0.8, MAT.amberGlow, 0.04);
    a.rotation.z = sg * Math.PI / 2; // left (sg -1) points +x
    a.position.set(sg * 2.35, 1.35, 0.1);
    B.add(a);
    return a;
  });

  // ---------- the load: the teacher's follow-up ----------
  function weight() {
    const g = new THREE.Group();
    g.add(at(box(0.95, 0.34, 0.75, MAT.ivory3, 0.06), 0, 0.17, 0));
    g.add(at(box(0.6, 0.04, 0.08, MAT.mintGlow, 0.01), 0, 0.2, 0.38));
    return g;
  }
  const loadA = weight(), loadB = weight();
  wA.add(loadA);
  wB.add(loadB);
  const dust = Array.from({ length: 14 }, (_, i) => {
    const d = mesh(new THREE.SphereGeometry(0.03 + hash(i + 40) * 0.03, 8, 6), MAT.ivory3, { cast: false });
    A.add(d);
    return d;
  });
  const ringA = at(mesh(new THREE.TorusGeometry(1.15, 0.03, 10, 80), MAT.amberGlow, { cast: false }), 0, 0.57, 0);
  ringA.rotation.x = Math.PI / 2;
  A.add(ringA);
  G.add(A, B);

  // ---------- labels ----------
  const L = {
    tA: label({ html: '<b>A’s reasoning</b>', anchor: [-2.7, 0.32, 1.1], align: 'below', dy: 6, cls: 'tag ivory' }),
    tB: label({ html: '<b>B’s reasoning</b>', anchor: [2.7, 0.32, 1.1], align: 'below', dy: 6, cls: 'tag ivory' }),
    fA: label({ html: 'AI users get higher grades', anchor: [-3.6, 0.6, 0.6], dx: -8, align: 'right', cls: 'tag small' }),
    jA: label({ html: '→ so AI caused them', anchor: [-3.55, 0.92, 0.5], dx: -8, align: 'right', cls: 'tag small amber' }),
    topA: label({ html: 'polished conclusion', anchor: u3, dy: -16, cls: 'tag small' }),
    // the two inferences A's model stacks, kept apart: the first is what fails under load
    chain: overlay('<div class="lead chain">AI use <b class="amber">→?</b> higher grades <b>→?</b> lasting learning</div>', 'center-top', 'AI 使用 →？ 成绩更高 →？ 持久的学习'),
    deckB: label({ html: 'Claim: AI helps learning', anchor: deckTop, dy: -16, cls: 'tag small' }),
    pilB: label({ html: 'evidence (thin)', anchor: pillars[1], dx: 30, align: 'left', cls: 'tag small' }),
    slotB: label({ html: 'answer to other explanations? <i>missing</i>', anchor: slots[1], dx: 20, dy: 20, align: 'left', cls: 'tag small amber' }),
    f1: label({ html: 'vague sentence', anchor: pinsA[3].userData.head, dy: -8, cls: 'tag tiny' }),
    f2: label({ html: 'missing citation', anchor: pinsB[3].userData.head, dy: -8, cls: 'tag tiny' }),
    f3: label({ html: 'awkward transition', anchor: pinsA[5].userData.head, dx: 8, align: 'left', cls: 'tag tiny' }),
    load: label({ html: '<b>Load test:</b> the teacher’s follow-up question', anchor: [0, 2.9, 0], cls: 'tag mint', zh: '负荷测试：老师的追问' }),
    decA: label({ html: '<b>Decisive for A</b><br>correlation vs. causation', anchor: [-2.7, 2.85, 0], cls: 'tag amber big', zh: 'A 的决定性问题：相关与因果' }),
    decB: label({ html: '<b>Decisive for B</b><br>stronger evidence · answer rival explanations', anchor: [2.7, 2.55, 0], cls: 'tag amber big', zh: 'B 的决定性问题：更有力的证据 · 回应其他解释' }),
    alt1: label({ html: 'stronger students may use AI more', anchor: alts[0], dy: -6, cls: 'tag small amber' }),
    alt2: label({ html: 'AI-allowed tasks may be easier to score well on', anchor: alts[1], dy: -6, cls: 'tag small amber' }),
    changes: label({ html: 'Fixing it changes how the other flaws look', anchor: [-2.7, 2.3, 0], cls: 'tag mint' }),
    legend: overlay(`<div class="legend-card"><div><i class="dot amber"></i><b>Decisive</b>: found by testing, reshapes the rest</div><div><i class="dot dim"></i><b>Minor</b>: real flaws, not the priority now</div><p><b>Principal contradiction</b>: the claim outruns its evidence.</p></div>`, 'center-top', '主要矛盾：结论超出了证据所能支撑的范围'),
  };

  function update(lt) {
    const t = lt;
    // assemble block by block
    rise(A, ramp(t, 0.2, 2.2), 1.0);
    rise(B, ramp(t, 0.5, 2.5), 1.0);
    pop(fA, ramp(t, 0.6, 1.2));
    pop(jL, ramp(t, 1.0, 1.5)); pop(jR, ramp(t, 1.1, 1.6));
    pop(u1, ramp(t, 1.4, 1.9)); pop(u2, ramp(t, 1.7, 2.2)); pop(u3, ramp(t, 2.0, 2.6));
    pop(slab, ramp(t, 0.8, 1.3));
    pillars.forEach((p, i) => pop(p, ramp(t, 1.2 + i * 0.15, 1.7 + i * 0.15)));
    pop(deck, ramp(t, 1.7, 2.2)); pop(deckTop, ramp(t, 2.0, 2.5));

    // minor flaws
    const sB = c['4b'].s;
    const sI = c['4i'].s, sJ = c['4j'].s;
    const dimK = ramp(t, sJ, sJ + 1.2);
    pinsA.forEach((p, i) => {
      const gone = fadesWithFix.includes(i) ? ramp(t, sI + 2.0 + i * 0.2, sI + 2.6 + i * 0.2) : 0;
      pop(p, ramp(t, sB + 0.3 + i * 0.25, sB + 0.7 + i * 0.25) * (1 - gone));
      setGlow(p.userData.head, 0.7 * (1 - dimK) + 0.05, dimK > 0.5 ? COLORS.ivory3 : COLORS.amber);
    });
    pinsB.forEach((p, i) => {
      pop(p, ramp(t, sB + 0.5 + i * 0.25, sB + 0.9 + i * 0.25));
      setGlow(p.userData.head, 0.7 * (1 - dimK) + 0.05, dimK > 0.5 ? COLORS.ivory3 : COLORS.amber);
    });

    // load descends
    const sC = c['4c'].s;
    const kl = ramp(t, sC + 1.2, sC + 3.0, ease.in);
    for (const [w, k] of [[loadA, kl], [loadB, ramp(t, sC + 1.5, sC + 3.3, ease.in)]]) {
      w.visible = k > 0;
      w.position.y = (1 - k) * 2.6;
    }
    const impact = (t > sC + 3.0 && t < sC + 3.8) ? Math.sin((t - sC - 3.0) * 30) * 0.015 * (1 - (t - sC - 3.0) / 0.8) : 0;

    // A fails at the joint in one decisive drop: the left joint block is knocked
    // out and tumbles off the foundation, the tower lurches, bounces and settles
    // crooked. Later it is rebuilt with a mint brace.
    const sD = c['4d'].s, tHit = sD + 0.95;
    const fail = ramp(t, sD + 0.4, tHit, ease.in);
    const fix = ramp(t, sI + 0.8, sI + 2.2);
    const crackOpen = fail * (1 - fix);
    const kick = ramp(t, sD + 0.5, sD + 1.1, ease.out), tumble = ramp(t, sD + 0.9, sD + 1.5, ease.in);
    offset(jL, -0.88 * kick, -0.05 * kick - 0.45 * tumble, 0.12 * kick);
    jL.rotation.z = 0.4 * kick + (Math.PI - 0.4) * tumble; // lands flat, upside down
    offset(jR, 0.08 * crackOpen, 0, 0);
    crack.visible = fail > 0.02 && fix < 0.98;
    crack.scale.y = 0.4 + 0.6 * crackOpen;
    setGlow(crack, 2.6 + 3.0 * Math.exp(-Math.max(0, t - tHit) * 3) * (t > tHit ? 1 : 0));
    fade(jL, 1 - fix); fade(jR, 1 - fix);
    const since = t - tHit;
    const bounce = since > 0 ? Math.sin(since * 16) * Math.exp(-since * 4.5) * 0.06 : 0;
    const shake = Math.sin(t * 22) * 0.012 * win(t, sD + 0.4, sD + 2.6, 0.3);
    U.rotation.z = (0.2 * fail + bounce) * (1 - fix) + shake + impact;
    U.position.x = 0.85 - 0.14 * crackOpen;
    U.position.y = 0.64 - 0.08 * crackOpen;
    const kd = clamp01(since / 1.4);
    dust.forEach((d, i) => {
      d.visible = kd > 0 && kd < 1;
      if (!d.visible) return;
      const a = hash(i) * Math.PI * 2, sp = 0.35 + hash(i + 20) * 0.45, r = sp * ease.out(kd);
      d.position.set(-0.2 + Math.cos(a) * r, 0.57 + 0.5 * kd * sp - 0.45 * kd * kd, 0.25 + Math.sin(a) * r * 0.6);
      fade(d, 1 - kd);
    });
    pop(brace, ramp(t, sI + 0.4, sI + 1.4));
    offset(brace, 0, 0, (1 - ramp(t, sI + 0.4, sI + 1.6, ease.out)) * 1.6);
    const pulse = 0.5 + 0.5 * Math.sin(t * 4);
    const ra = win(t, sD + 1.0, sI + 1.6, 0.5);
    ringA.visible = ra > 0.01;
    if (ringA.visible) { fade(ringA, ra * (0.5 + 0.5 * pulse)); ringA.scale.setScalar(1 + 0.05 * pulse); }

    // B sways; rival explanations push from both sides; then sleeves + buttresses
    const sF = c['4f'].s, sH = c['4h'].s;
    const steady = ramp(t, sI + 1.0, sI + 3.0);
    const amp = (0.03 * ramp(t, sF, sF + 1.5) + 0.03 * ramp(t, sH, sH + 1.5)) * (1 - steady);
    S.rotation.z = Math.sin(t * 2.6) * amp + impact;
    alts.forEach((a, i) => {
      const k = win(t, sH + i * 1.6, sI + 1.2, 0.5);
      pop(a, k);
      const push = 0.5 + 0.5 * Math.sin(t * 2.6 + (i ? 0 : Math.PI));
      a.position.x = (i === 0 ? -2.35 : 2.35) + (i === 0 ? 1 : -1) * push * 0.12;
    });
    const gs = win(t, c['4g'].s, sI + 1.8, 0.4);
    slots.forEach((s) => (s.visible = gs > 0.01 && (Math.sin(t * 5) > -0.4 || t > sI)));
    ghostMat.opacity = gs;
    buttresses.forEach((b, i) => {
      pop(b, ramp(t, sI + 1.0 + i * 0.3, sI + 1.8 + i * 0.3));
    });
    sleeves.forEach((s, i) => {
      s.visible = t > sI + 1.4;
      s.scale.set(1, Math.max(0.001, ramp(t, sI + 1.4 + i * 0.2, sI + 2.4 + i * 0.2)), 1);
      s.position.y = 0.5 * s.scale.y;
    });

    // labels
    const all = win(t, 2.6, 61, 0.5);
    L.tA.set(all); L.tB.set(all);
    L.fA.set(win(t, 2.8, sD, 0.4) + win(t, sD + 2.0, c['4f'].s, 0.4));
    L.jA.set(win(t, 3.2, c['4f'].s, 0.4));
    L.topA.set(win(t, 3.2, sB + 0.2, 0.4));
    L.deckB.set(win(t, 3.4, sB + 0.2, 0.4) + win(t, sF, c['4g'].s + 0.3, 0.4));
    L.pilB.set(win(t, c['4g'].s, sI + 2.4, 0.4));
    L.slotB.set(win(t, c['4g'].s + 1.0, sH, 0.4));
    L.f1.set(win(t, sB + 1.5, sC + 0.4, 0.3));
    L.f2.set(win(t, sB + 2.8, sC + 0.4, 0.3));
    L.f3.set(win(t, sB + 4.0, sC + 0.4, 0.3));
    L.load.set(win(t, sC + 0.6, sD, 0.4));
    L.chain.set(win(t, sD + 0.6, c['4f'].s, 0.5));
    L.decA.set(win(t, c['4e'].s, c['4f'].s + 0.8, 0.5)); // only while the camera is on A
    L.decB.set(win(t, c['4g'].s + 0.4, sI + 0.4, 0.5));
    L.alt1.set(win(t, sH + 0.4, sI, 0.4));
    L.alt2.set(win(t, sH + 2.0, sI, 0.4));
    L.changes.set(win(t, sI + 2.2, sJ + 0.2, 0.5));
    L.legend.set(win(t, sJ + 0.4, 61, 0.6));
  }

  const track = camTrack([
    { t: 0, p: [0, 6.6, 13.4], l: [0, 1.2, 0] },
    { t: c['4b'].s, p: [0, 5.8, 11.6], l: [0, 1.3, 0] },
    { t: c['4c'].e, p: [0, 5.6, 11.2], l: [0, 1.4, 0] },
    { t: c['4d'].s + 1.2, p: [-1.9, 4.7, 9.8], l: [-2.5, 1.35, 0] },
    { t: c['4e'].e, p: [-1.9, 4.7, 9.6], l: [-2.5, 1.35, 0] },
    { t: c['4f'].s + 1.2, p: [1.9, 4.7, 9.8], l: [2.5, 1.35, 0] },
    { t: c['4h'].e, p: [1.9, 4.7, 9.6], l: [2.5, 1.35, 0] },
    { t: c['4i'].s + 1.6, p: [0, 5.8, 11.6], l: [0, 1.3, 0] },
    { t: 60, p: [0, 6.0, 12.0], l: [0, 1.3, 0] },
  ]);
  // a short, decaying jolt when A's joint gives way
  const camera = (t, p, l) => {
    track(t, p, l);
    const k = t - (c['4d'].s + 0.95);
    if (k > 0 && k < 1) {
      const a = Math.exp(-k * 5) * 0.07;
      p.x += Math.sin(k * 47) * a;
      p.y += Math.sin(k * 61 + 1) * a;
    }
  };

  return { update, camera };
}
