// The diorama world: seven chapter sets placed around a ring, shared lighting,
// fog and ground, and a camera that glides from one set to the next.
// Everything is computed from the master time, so seeking is exact.
import * as THREE from 'three';
import { CHAPTERS, chapterAt, localCues } from './script.js';
import { COLORS, ease, lerp, clamp01, hash } from './kit.js';
import * as c1 from './chapters/c1.js';
import * as c2 from './chapters/c2.js';
import * as c3 from './chapters/c3.js';
import * as c4 from './chapters/c4.js';
import * as c5 from './chapters/c5.js';
import * as c6 from './chapters/c6.js';
import * as c7 from './chapters/c7.js';

const MODULES = [c1, c2, c3, c4, c5, c6, c7];
const RING_R = 44.6;
const TRANSITION = 3.2; // seconds of camera glide at the start of each chapter
const FOG_NEAR = 26, FOG_FAR = 72;
const HEMI = 0.85, KEY = 1.8, RIM = 0.5;

const freshEnv = () => ({ dim: 0, sun: 0, fogFar: 0 });

export class World {
  constructor(labels) {
    const scene = (this.scene = new THREE.Scene());
    scene.background = new THREE.Color(COLORS.bg);
    scene.fog = new THREE.Fog(COLORS.bg, FOG_NEAR, FOG_FAR);

    // ground and the faint path that links the sets
    const ground = new THREE.Mesh(new THREE.CircleGeometry(160, 96), new THREE.MeshStandardMaterial({ color: COLORS.ground, roughness: 1 }));
    ground.rotation.x = -Math.PI / 2;
    ground.position.y = -0.5;
    ground.receiveShadow = true;
    scene.add(ground);
    const path = new THREE.Mesh(
      new THREE.TorusGeometry(RING_R, 0.03, 6, 360),
      new THREE.MeshBasicMaterial({ color: COLORS.mint, transparent: true, opacity: 0.12, fog: true }),
    );
    path.rotation.x = Math.PI / 2;
    path.position.y = -0.48;
    scene.add(path);

    // drifting dust for depth (rotates with time; no per-frame state). Seeded, so every
    // load and every review frame shows the same field.
    const N = 900;
    const pos = new Float32Array(N * 3);
    for (let i = 0; i < N; i++) {
      const a = hash(i * 3 + 1) * Math.PI * 2, r = RING_R + (hash(i * 3 + 2) - 0.5) * 50;
      pos[i * 3] = Math.cos(a) * r;
      pos[i * 3 + 1] = hash(i * 3 + 3) * 14 - 0.5;
      pos[i * 3 + 2] = Math.sin(a) * r;
    }
    const dg = new THREE.BufferGeometry();
    dg.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    this.dust = new THREE.Points(dg, new THREE.PointsMaterial({ color: 0xf1e7d3, size: 0.06, transparent: true, opacity: 0.35, depthWrite: false }));
    scene.add(this.dust);

    // lights
    this.hemi = new THREE.HemisphereLight(0xe6fff7, 0x0a2a2d, HEMI);
    scene.add(this.hemi);
    const key = (this.key = new THREE.DirectionalLight(0xfff0dc, KEY));
    key.castShadow = true;
    key.shadow.mapSize.set(2048, 2048);
    Object.assign(key.shadow.camera, { left: -14, right: 14, top: 14, bottom: -14, near: 1, far: 70 });
    key.shadow.bias = -0.0004;
    key.shadow.normalBias = 0.025;
    scene.add(key, key.target);
    this.rim = new THREE.DirectionalLight(0x7fe3c3, RIM);
    scene.add(this.rim, this.rim.target);

    // chapter sets around the ring; local +z faces outward, toward the camera
    this.env = freshEnv();
    this.sets = MODULES.map((mod, i) => {
      const theta = (i * Math.PI * 2) / MODULES.length;
      const group = new THREE.Group();
      group.position.set(Math.sin(theta) * RING_R, 0, Math.cos(theta) * RING_R);
      group.rotation.y = theta;
      scene.add(group);
      group.updateMatrixWorld(true);
      const ctx = {
        group,
        cues: localCues(i),
        env: this.env,
        label: (o) => {
          let anchor = o.anchor;
          if (Array.isArray(anchor)) {
            const a = new THREE.Object3D();
            a.position.set(...anchor);
            group.add(a);
            anchor = a;
          }
          return labels.add({ ...o, anchor, chapter: i });
        },
        overlay: (html, pos, zh) => labels.add({ html, zh, cls: `ov ${pos}`, overlay: true, chapter: i }),
      };
      const api = mod.build(ctx);
      return { group, api, ch: CHAPTERS[i], dur: CHAPTERS[i].end - CHAPTERS[i].start };
    });

    this._p = new THREE.Vector3();
    this._l = new THREE.Vector3();
    this._p0 = new THREE.Vector3();
    this._l0 = new THREE.Vector3();
    this.activeLabels = new Map();
    this.dip = 0;
  }

  /** Bring every visible set to the state for master time t. */
  update(t) {
    const ci = chapterAt(t);
    const cur = this.sets[ci];
    const lt = t - cur.ch.start;
    const n = this.sets.length;
    const inTransition = ci > 0 && lt < TRANSITION;
    const prevI = (ci + n - 1) % n, nextI = (ci + 1) % n;

    this.sets.forEach((s, i) => {
      s.group.visible = i === ci || i === prevI || i === nextI;
    });

    // neighbours hold their end (previous) or start (next) state
    Object.assign(this.env, freshEnv());
    if (nextI !== ci) this.sets[nextI].api.update(nextI > ci ? 0 : this.sets[nextI].dur);
    let envPrev = null;
    Object.assign(this.env, freshEnv());
    if (prevI !== ci) {
      this.sets[prevI].api.update(prevI < ci ? this.sets[prevI].dur : 0);
      envPrev = { ...this.env };
    }
    Object.assign(this.env, freshEnv());
    cur.api.update(lt);
    if (inTransition && envPrev) {
      const k = ease.inOut(lt / TRANSITION);
      for (const key of Object.keys(this.env)) this.env[key] = lerp(envPrev[key], this.env[key], k);
    }

    // which chapters' labels are visible, and how strongly
    this.activeLabels.clear();
    this.activeLabels.set(ci, 1);
    if (inTransition) this.activeLabels.set(ci - 1, 1 - clamp01(lt / 0.8));

    // a soft dip toward the background mid-glide, so the half-built next set is never the subject
    this.dip = inTransition ? 0.72 * Math.pow(Math.sin(Math.PI * clamp01(lt / TRANSITION)), 2) : 0;

    this.dust.rotation.y = t * 0.004;
    this.dust.position.y = Math.sin(t * 0.1) * 0.3;
    this.chapterIndex = ci;
    this.localTime = lt;
  }

  /** Compute the camera pose for time t; returns {p, l} in world space. */
  cameraPose(t, aspect) {
    const ci = chapterAt(t);
    const cur = this.sets[ci];
    const lt = t - cur.ch.start;
    const p = this._p, l = this._l;
    cur.api.camera(lt, p, l);
    cur.group.localToWorld(p);
    cur.group.localToWorld(l);
    if (ci > 0 && lt < TRANSITION) {
      const prev = this.sets[ci - 1];
      prev.api.camera(prev.dur, this._p0, this._l0);
      prev.group.localToWorld(this._p0);
      prev.group.localToWorld(this._l0);
      // the gaze leads: turn toward the next set first, then travel to it
      const kp = ease.inOut(lt / TRANSITION);
      const kl = ease.inOut(Math.min(1, lt / (TRANSITION * 0.62)));
      p.lerpVectors(this._p0, p, kp);
      l.lerpVectors(this._l0, l, kl);
      p.y += Math.sin(Math.PI * kp) * 1.4; // a gentle lift between sets
    }
    // narrower screens: step back so the set still fits
    const fit = Math.min(2.2, Math.pow(Math.max(1, (16 / 9) / aspect), 0.8));
    if (fit > 1) p.sub(l).multiplyScalar(fit).add(l);
    return { p, l };
  }

  /** Lights and fog follow the camera target. */
  applyEnvironment(lookAt) {
    const env = this.env;
    const ci = this.chapterIndex;
    const g = this.sets[ci].group;
    const sunA = g.rotation.y + 0.55 + env.sun * 1.3;
    this.key.position.set(lookAt.x + Math.sin(sunA) * 9, lookAt.y + 15, lookAt.z + Math.cos(sunA) * 9);
    this.key.target.position.copy(lookAt);
    this.rim.position.set(lookAt.x - Math.sin(g.rotation.y) * 10, lookAt.y + 6, lookAt.z - Math.cos(g.rotation.y) * 10);
    this.rim.target.position.copy(lookAt);
    const d = clamp01(env.dim);
    this.hemi.intensity = HEMI * (1 - 0.65 * d);
    this.key.intensity = KEY * (1 - 0.75 * d) * (1 - 0.25 * Math.abs(env.sun));
    this.scene.fog.near = FOG_NEAR + env.fogFar * 0.5;
    this.scene.fog.far = FOG_FAR + env.fogFar;
  }
}
