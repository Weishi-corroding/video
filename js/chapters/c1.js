// 01 One Tool, Two Approaches
// Two miniature study spaces share one floating AI interface. A requests a
// finished answer; B requests a critique of a first attempt. The teacher's
// follow-up exposes a different gap in each student's reasoning.
import {
  THREE, MAT, COLORS, at, box, mesh, glow, plinth, studySpace, aiPanel, packet, card, figure,
  tube, pop, fade, rise, ramp, win, ease, camTrack, setGlow, offset, clamp01,
} from '../kit.js';

export function build(ctx) {
  const { group: G, label, overlay, cues: c } = ctx;
  G.add(plinth(7.6));

  const A = at(studySpace({ seed: 1, who: 'A' }), -3.1, 0, 0.5);
  A.rotation.y = 0.42;
  const B = at(studySpace({ seed: 2, who: 'B' }), 3.1, 0, 0.5);
  B.rotation.y = -0.42;
  G.add(A, B);

  const ai = at(aiPanel(), 0, 3.0, -1.4);
  G.add(ai);

  // threads from the AI panel to each laptop
  const lapA = new THREE.Vector3(), lapB = new THREE.Vector3();
  G.updateMatrixWorld(true);
  const toLocal = (obj, out) => G.worldToLocal(obj.getWorldPosition(out));
  toLocal(A.userData.laptop, lapA).y += 0.2;
  toLocal(B.userData.laptop, lapB).y += 0.2;
  const threadMat = glow(COLORS.amber, 0.7);
  const thA = tube([[-0.7, 2.5, -1.35], [-1.9, 2.3, -0.6], [lapA.x, lapA.y + 0.5, lapA.z], [lapA.x, lapA.y, lapA.z]], 0.014, threadMat);
  const thB = tube([[0.7, 2.5, -1.35], [1.9, 2.3, -0.6], [lapB.x, lapB.y + 0.5, lapB.z], [lapB.x, lapB.y, lapB.z]], 0.014, threadMat);
  G.add(thA, thB);

  // A: request up, finished answer down
  const reqA = packet(MAT.paper, 0.13);
  const ansA = card({ w: 0.46, h: 0.6, accent: MAT.amberGlow });
  const ansGlow = at(mesh(new THREE.PlaneGeometry(0.62, 0.76), glow(COLORS.amber, 0.9), { cast: false }), 0, -0.02, 0);
  ansGlow.rotation.x = -Math.PI / 2;
  ansA.add(ansGlow);
  G.add(reqA, ansA);
  const ansRest = new THREE.Vector3();
  toLocal(A.userData.paper, ansRest).y += 0.04;

  // B: draft lines appear, request up, critique flags down
  const draftLines = B.userData.paper.userData.lines;
  const reqB = packet(MAT.paper, 0.13);
  G.add(reqB);
  const flags = [];
  const paperB = B.userData.paper;
  for (let i = 0; i < 3; i++) {
    const f = new THREE.Group();
    f.add(at(box(0.012, 0.16, 0.012, MAT.ivory3, 0.003), 0, 0.08, 0));
    f.add(at(box(0.09, 0.055, 0.008, i === 1 ? MAT.amberGlow : MAT.mintGlow, 0.006), 0.045, 0.14, 0));
    f.position.set(0.12 - i * 0.02, 0.012, -0.12 + i * 0.12);
    paperB.add(f);
    flags.push(f);
  }
  const critPkts = [0, 1, 2].map(() => { const p = packet(MAT.mintGlow, 0.08); G.add(p); return p; });
  const flagTargets = flags.map(() => new THREE.Vector3());

  // Teacher in the foreground, facing the students
  const teacher = at(figure({ seated: false, scale: 1.12, mat: MAT.ivory2 }), 0, 0, 3.4);
  teacher.rotation.y = Math.PI;
  G.add(teacher);
  const wave = at(mesh(new THREE.TorusGeometry(1, 0.02, 8, 96), MAT.mintGlow, { cast: false }), 0, 1.25, 3.4);
  wave.rotation.x = Math.PI / 2;
  G.add(wave);

  // Reasoning chains above each desk
  function chain(x, kinds) {
    const g = new THREE.Group();
    const parts = [];
    kinds.forEach((k, i) => {
      const px = (i - (kinds.length - 1) / 2) * 0.98;
      let m;
      if (k === 'gap') {
        m = new THREE.LineSegments(new THREE.EdgesGeometry(new THREE.BoxGeometry(0.56, 0.3, 0.22)), new THREE.LineBasicMaterial({ color: COLORS.amber }));
      } else if (k === 'thin') {
        m = box(0.56, 0.08, 0.22, MAT.ivory3, 0.02);
      } else {
        m = box(0.56, 0.3, 0.22, MAT.ivory, 0.04);
      }
      m.position.x = px;
      g.add(m);
      parts.push(m);
      if (i > 0) {
        const link = at(box(0.42, 0.03, 0.03, MAT.ivory3, 0.01), px - 0.49, 0, 0);
        g.add(link);
      }
    });
    g.position.set(x, 2.25, 0.3);
    G.add(g);
    return { g, parts };
  }
  const chA = chain(-3.0, ['solid', 'gap', 'solid']);
  const chB = chain(3.0, ['solid', 'thin', 'gap']);

  // hesitation marks ride on the heads, so they follow the tilt
  const thinkAt = (space) => {
    const o = at(new THREE.Object3D(), 0, 0.42, 0);
    space.userData.figure.userData.head.add(o);
    return o;
  };
  const thinkA = thinkAt(A), thinkB = thinkAt(B);

  // ---------- labels ----------
  const L = {
    drama: overlay(`<div class="badge-big"><b>Dramatization</b><span>Fictional students · no real data</span></div>`, 'center-top', '情景演绎 · 虚构学生 · 无真实数据'),
    question: overlay(`<div class="question-card"><small>The question</small>If students who use AI receive higher grades, does that prove AI improves learning?</div>`, 'center-mid', '如果使用 AI 的学生成绩更高，这能否证明 AI 促进了学习？'),
    tagA: label({ html: '<b>Student A</b>', anchor: [-3.4, 1.9, 0.0], cls: 'tag ivory' }),
    tagB: label({ html: '<b>Student B</b>', anchor: [3.4, 1.9, 0.0], cls: 'tag ivory' }),
    ai: label({ html: 'Shared AI tool', anchor: [0, 3.75, -1.4], cls: 'tag amber' }),
    askA: label({ html: '“Give me a finished answer.”', anchor: [-3.0, 2.15, 0.4], cls: 'speech' }),
    gotA: label({ html: '<b>Finished answer</b><br>fast · fluent · organized', anchor: ansA, dy: -18, cls: 'tag amber' }),
    askB: label({ html: '“Here’s my first attempt. Critique it.”', anchor: [3.0, 2.15, 0.4], cls: 'speech' }),
    gotB: label({ html: '<b>Critique</b><br>questions · gaps · counterexamples', anchor: [4.2, 0.9, 1.5], dx: 20, align: 'left', cls: 'tag mint' }),
    teacher: label({ html: '“Walk me through your reasoning.”', anchor: [0, 1.75, 3.4], cls: 'speech teacher' }),
    tchTag: label({ html: 'Teacher', anchor: [0, 0.0, 3.4], dy: 34, cls: 'tag ivory small' }),
    chA0: label({ html: 'higher grades', anchor: chA.parts[0], dy: 20, align: 'below', cls: 'tag small' }),
    chA2: label({ html: 'AI improves learning', anchor: chA.parts[2], dy: 20, align: 'below', cls: 'tag small' }),
    gapA: label({ html: '<b>Gap:</b> correlation ≠ causation', anchor: chA.parts[1], dy: -24, cls: 'tag amber' }),
    chB0: label({ html: 'plausible claim', anchor: chB.parts[0], dy: 20, align: 'below', cls: 'tag small' }),
    chB2: label({ html: 'other explanations?', anchor: chB.parts[2], dy: 20, align: 'below', cls: 'tag small amber' }),
    gapB: label({ html: '<b>Gap:</b> thin evidence, rivals unanswered', anchor: chB.parts[1], dy: -24, cls: 'tag amber' }),
    thinkA: label({ html: '···', anchor: thinkA, cls: 'tag ivory thought' }),
    thinkB: label({ html: '···', anchor: thinkB, cls: 'tag ivory thought' }),
  };

  const tmp = new THREE.Vector3();
  const along = (curve, k, out) => curve.getPointAt(clamp01(k), out);

  function update(lt) {
    const t = lt;
    // assemble
    rise(A, ramp(t, 0.2, 2.6), 1.2);
    rise(B, ramp(t, 0.6, 3.0), 1.2);
    pop(ai, ramp(t, 1.4, 3.2));
    ai.position.y = 3.0 + Math.sin(t * 0.9) * 0.06;
    ai.userData.animate(t);
    thA.userData.progress(ramp(t, 2.6, 4.4));
    thB.userData.progress(ramp(t, 2.8, 4.6));

    // A requests a finished answer
    const sA = c['1c'].s;
    const kq = ramp(t, sA + 0.6, sA + 2.0, ease.inOut);
    reqA.visible = kq > 0 && kq < 1;
    if (reqA.visible) along(thA.userData.curve, 1 - kq, reqA.position);
    const kd = ramp(t, sA + 2.2, sA + 4.0, ease.inOut);
    ansA.visible = kd > 0;
    if (ansA.visible) {
      along(thA.userData.curve, 1 - kd * 0.92, tmp);
      ansA.position.lerpVectors(tmp, ansRest, ramp(t, sA + 3.6, sA + 4.6));
      const s = 0.4 + 0.6 * ramp(t, sA + 2.2, sA + 4.4, ease.out);
      ansA.scale.setScalar(s);
      ansA.rotation.y = 0.42 + (1 - ramp(t, sA + 3.6, sA + 4.6)) * 0.6;
    }
    setGlow(ansGlow, 0.5 + 0.6 * win(t, sA + 4.2, c['1e'].s, 1.0));
    fade(ansGlow, 0.25 + 0.35 * ramp(t, sA + 4.2, sA + 5.0));

    // B drafts, then asks for critique
    const sB = c['1d'].s;
    draftLines.forEach((l, i) => {
      const k = ramp(t, sB + 0.2 + i * 0.35, sB + 0.7 + i * 0.35, ease.out);
      l.visible = k > 0.01;
      l.scale.x = Math.max(k, 0.01);
    });
    // before B's turn the paper lines are hidden (blank page)
    const kqB = ramp(t, sB + 2.2, sB + 3.4);
    reqB.visible = kqB > 0 && kqB < 1;
    if (reqB.visible) along(thB.userData.curve, 1 - kqB, reqB.position);
    G.updateMatrixWorld(true);
    flags.forEach((f, i) => {
      toLocal(f, flagTargets[i]);
      const k = ramp(t, sB + 3.6 + i * 0.25, sB + 4.8 + i * 0.25);
      const p = critPkts[i];
      p.visible = k > 0 && k < 1;
      if (p.visible) {
        along(thB.userData.curve, 1 - k * 0.9, tmp);
        p.position.lerpVectors(tmp, flagTargets[i], ramp(k, 0.6, 1));
      }
      pop(f, ramp(t, sB + 4.6 + i * 0.25, sB + 5.1 + i * 0.25));
    });

    // Teacher's follow-up
    const sT = c['1e'].s;
    rise(teacher, ramp(t, sT - 0.6, sT + 0.8) * (1 - ramp(t, c['1f'].s + 0.6, c['1f'].s + 2.2)), 1.4);
    const kw = clamp01((t - (sT + 1.6)) / 2.4);
    wave.visible = kw > 0 && kw < 1;
    if (wave.visible) { wave.scale.setScalar(0.3 + kw * 4.2); fade(wave, (1 - kw) * 0.9); }

    // Hesitation: heads tilt, at different moments
    const sH = c['1f'].s;
    const hA = win(t, sH, 45, 0.6), hB = win(t, sH + 0.5, 45, 0.6);
    A.userData.figure.userData.head.rotation.z = hA * (0.36 + Math.sin(t * 2.2) * 0.06);
    A.userData.figure.userData.head.rotation.x = 0.12 * hA;
    B.userData.figure.userData.head.rotation.z = -hB * (0.3 + Math.sin(t * 1.7 + 1) * 0.05);
    B.userData.figure.userData.head.rotation.x = 0.22 * hB;

    // Reasoning chains
    const sG = c['1g'].s, sR = c['1h'].s;
    chA.parts.forEach((p, i) => pop(p, ramp(t, sH + 0.2 + i * 0.3, sH + 0.9 + i * 0.3)));
    chA.g.children.forEach((m) => { if (!chA.parts.includes(m)) pop(m, ramp(t, sH + 0.6, sH + 1.4)); });
    chB.parts.forEach((p, i) => pop(p, ramp(t, sH + 0.6 + i * 0.3, sH + 1.3 + i * 0.3)));
    chB.g.children.forEach((m) => { if (!chB.parts.includes(m)) pop(m, ramp(t, sH + 1.0, sH + 1.8)); });
    const flick = 0.6 + 0.4 * Math.sin(t * 6);
    chA.parts[1].material.opacity = 1;
    chA.parts[1].material.color.setHex(t > sG ? COLORS.amber : COLORS.ivory3);
    chA.parts[1].visible = chA.parts[1].visible && (t < sG || flick > 0.25);
    chB.parts[2].material.color.setHex(t > sR ? COLORS.amber : COLORS.ivory3);
    chA.g.position.y = 2.25 + Math.sin(t * 1.1) * 0.03;
    chB.g.position.y = 2.25 + Math.sin(t * 1.1 + 1.5) * 0.03;

    // labels
    L.drama.set(win(t, 0.6, c['1b'].s - 0.2, 0.6));
    L.question.set(win(t, c['1b'].s, c['1c'].s - 0.2, 0.6));
    // name tags step aside while the question card is up
    const sQ = c['1b'].s;
    // name tags step aside while their own student is speaking
    const askA = win(t, sA + 0.2, sA + 3.0, 0.4), askB = win(t, sB + 1.6, sB + 4.4, 0.4);
    L.tagA.set((win(t, 2.4, sQ, 0.4) + win(t, sA - 0.6, c['1f'].s, 0.6)) * (1 - askA));
    L.tagB.set((win(t, 2.8, sQ, 0.4) + win(t, sA - 0.6, c['1f'].s, 0.6)) * (1 - askB));
    L.ai.set(win(t, 3.4, sQ, 0.4));
    L.askA.set(askA);
    L.gotA.set(win(t, sA + 4.4, sB + 0.4, 0.5));
    L.askB.set(askB);
    L.gotB.set(win(t, sB + 5.0, sT + 0.4, 0.5));
    L.teacher.set(win(t, sT + 0.6, sH + 0.3, 0.4));
    L.tchTag.set(win(t, sT + 0.6, sH + 0.3, 0.4));
    const chainA = win(t, sH + 1.0, 45.5, 0.5), chainB = win(t, sH + 1.4, 45.5, 0.5);
    L.chA0.set(chainA); L.chA2.set(chainA);
    L.chB0.set(chainB); L.chB2.set(chainB);
    L.gapA.set(win(t, sG + 0.4, 45.5, 0.5));
    L.gapB.set(win(t, sR + 0.4, 45.5, 0.5));
    L.thinkA.set(win(t, sH + 0.1, sH + 2.8, 0.3));
    L.thinkB.set(win(t, sH + 0.6, sH + 3.0, 0.3));
  }

  const camera = camTrack([
    { t: 0, p: [0, 9.5, 19], l: [0, 1.2, 0] },
    { t: 5, p: [0, 6.2, 13.4], l: [0, 1.6, -0.2] },
    { t: c['1c'].s, p: [0, 5.8, 12.8], l: [0, 1.7, -0.3] },
    { t: c['1c'].s + 3, p: [-1.4, 5.4, 12.0], l: [-1.4, 1.6, -0.2] },
    { t: c['1d'].s + 1, p: [1.4, 5.4, 12.0], l: [1.4, 1.6, -0.2] },
    { t: c['1e'].s + 1, p: [0, 6.0, 13.2], l: [0, 1.5, 0.2] },
    { t: c['1f'].s + 1, p: [0, 5.6, 12.6], l: [0, 1.9, 0] },
    { t: 45, p: [0, 5.4, 12.2], l: [0, 1.9, 0] },
  ]);

  return { update, camera };
}
