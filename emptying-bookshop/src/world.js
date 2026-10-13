// The world: renderer, light by time of day, the campus diorama, its crowd, Jia, and the camera rig.
// Everything here is a pure function of master time t, so seeking is exact.
import * as THREE from 'three';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';
import {
  PAL, clamp, lerp, smooth, smoother, rng, v3, mat, matS, glow, mesh, rbox, place, sign, figure, pose,
  shelf, counter, lampPost, tree, polyPath, FIG, pick,
} from './kit.js';

// Key places on the campus (x, z). North is -z.
export const P = {
  shop: v3(17, 0, -25),
  shopDoor: v3(14.6, 0, -21.6),
  shopFront: -22.25,
  library: v3(0, 0, -16),
  libraryFront: v3(0, 0, -10.6),
  oldShop: v3(7, 0, 4.2),
  teach1Door: v3(-18.6, 0, 0),
  teach2Door: v3(-18.6, 0, 7),
  dorm1Door: v3(-7, 0, 20.4),
  dorm2Door: v3(11, 0, 20.4),
  cafeDoor: v3(19.9, 0, 2),
  quad: v3(-6, 0, -4),
};

// Side path from the library plaza to the shop door.
export const SIDE_PATH = [[4.8, -10.4], [11, -10.4], [14.6, -13.6], [14.6, -21.6]];

// ---------------- lighting presets ----------------
const PRESETS = {
  day:      { sun: 0xfff0dc, sunI: 2.7, el: 40, az: 215, hemiS: 0xd6e4ff, hemiG: 0x6e5c48, hemiI: 1.15, top: 0x1d2a40, hor: 0x51647a, night: 0, stars: 0, exp: 1.0 },
  overcast: { sun: 0xe4e9f0, sunI: 1.25, el: 55, az: 200, hemiS: 0xdfe6ef, hemiG: 0x5d5a55, hemiI: 1.45, top: 0x262d38, hor: 0x5a6270, night: 0, stars: 0, exp: 0.98 },
  golden:   { sun: 0xffc48a, sunI: 2.6, el: 20, az: 240, hemiS: 0xf3d6c0, hemiG: 0x5e4a3c, hemiI: 0.95, top: 0x22253f, hor: 0x8a5e5a, night: 0.15, stars: 0, exp: 1.0 },
  dusk:     { sun: 0xff9f70, sunI: 1.4, el: 12, az: 250, hemiS: 0x8a8fc0, hemiG: 0x3a3038, hemiI: 0.85, top: 0x161b33, hor: 0x5e4058, night: 0.55, stars: 0.2, exp: 1.02 },
  evening:  { sun: 0xa9bcff, sunI: 0.75, el: 38, az: 150, hemiS: 0x5d6fa0, hemiG: 0x24222c, hemiI: 0.75, top: 0x0b1022, hor: 0x2b3050, night: 1, stars: 0.6, exp: 1.05 },
  night:    { sun: 0x9fb2ff, sunI: 0.55, el: 50, az: 160, hemiS: 0x4a5a8a, hemiG: 0x1c1b24, hemiI: 0.7, top: 0x070a18, hor: 0x1d2140, night: 1, stars: 1, exp: 1.05 },
  analysis: { sun: 0xffe2c4, sunI: 1.7, el: 30, az: 225, hemiS: 0xb9c6e6, hemiG: 0x3e3a40, hemiI: 1.0, top: 0x141b2c, hor: 0x3a4458, night: 0.35, stars: 0.1, exp: 1.0 },
};

const GRADE = /* glsl */`
uniform sampler2D tDiffuse;
uniform float sat, vig, fade, flash, focusAmt, focusR, aspect;
uniform vec2 focusC;
varying vec2 vUv;
void main() {
  vec4 c = texture2D(tDiffuse, vUv);
  float l = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
  vec2 d = (vUv - focusC) * vec2(aspect, 1.0);
  float outside = smoothstep(focusR, focusR * 1.35 + 0.02, length(d)) * focusAmt;
  float s = sat * (1.0 - 0.85 * outside);
  c.rgb = mix(vec3(l), c.rgb, s);
  c.rgb *= 1.0 - 0.72 * outside;
  vec2 v = vUv - 0.5;
  c.rgb *= 1.0 - vig * dot(v, v) * 1.5;
  c.rgb = mix(c.rgb, vec3(1.0), flash);
  c.rgb *= 1.0 - fade;
  gl_FragColor = c;
}`;

export function createWorld(container) {
  const renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: 'high-performance', preserveDrawingBuffer: false });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFShadowMap;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.0;
  container.appendChild(renderer.domElement);

  const scene = new THREE.Scene();
  scene.fog = new THREE.Fog(0x51647a, 110, 300);
  const camera = new THREE.PerspectiveCamera(40, 16 / 9, 0.1, 600);

  // Gradient sky dome.
  const skyMat = new THREE.ShaderMaterial({
    side: THREE.BackSide, depthWrite: false, fog: false,
    uniforms: { top: { value: new THREE.Color() }, hor: { value: new THREE.Color() } },
    vertexShader: 'varying vec3 vP; void main(){ vP = normalize(position); gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }',
    fragmentShader: 'uniform vec3 top, hor; varying vec3 vP; void main(){ float h = clamp(vP.y * 1.6 + 0.25, 0.0, 1.0); gl_FragColor = vec4(mix(hor, top, h), 1.0); }',
  });
  const sky = new THREE.Mesh(new THREE.SphereGeometry(400, 32, 16), skyMat);
  sky.renderOrder = -10;
  scene.add(sky);

  // Stars for the night scenes (deterministic positions).
  const starGeo = new THREE.BufferGeometry();
  {
    const r = rng(77), a = [];
    for (let i = 0; i < 700; i++) {
      const th = r() * Math.PI * 2, ph = Math.acos(lerp(0.08, 1, r()));
      const R = 380;
      a.push(R * Math.sin(ph) * Math.cos(th), R * Math.cos(ph), R * Math.sin(ph) * Math.sin(th));
    }
    starGeo.setAttribute('position', new THREE.Float32BufferAttribute(a, 3));
  }
  const starMat = new THREE.PointsMaterial({ color: 0xdfe6ff, size: 1.6, sizeAttenuation: false, transparent: true, opacity: 0, fog: false, depthWrite: false });
  scene.add(new THREE.Points(starGeo, starMat));

  const hemi = new THREE.HemisphereLight(0xd6e4ff, 0x6e5c48, 1.1);
  scene.add(hemi);
  const sun = new THREE.DirectionalLight(0xfff0dc, 2.6);
  sun.castShadow = true;
  sun.shadow.mapSize.set(2048, 2048);
  Object.assign(sun.shadow.camera, { left: -44, right: 44, top: 44, bottom: -44, near: 1, far: 220 });
  sun.shadow.bias = -0.0004;
  sun.shadow.normalBias = 0.03;
  sun.shadow.radius = 3;
  sun.target.position.set(0, 0, -2);
  scene.add(sun, sun.target);
  const fill = new THREE.DirectionalLight(0x9fb8ff, 0.35);
  fill.position.set(40, 30, 60);
  scene.add(fill);

  // Post-processing: bloom for lit windows and glowing threads, then a grade pass.
  const composer = new EffectComposer(renderer);
  composer.addPass(new RenderPass(scene, camera));
  const bloom = new UnrealBloomPass(new THREE.Vector2(512, 512), 0.32, 0.55, 0.86);
  composer.addPass(bloom);
  composer.addPass(new OutputPass());
  const grade = new ShaderPass({
    uniforms: {
      tDiffuse: { value: null }, sat: { value: 1 }, vig: { value: 0.55 }, fade: { value: 0 }, flash: { value: 0 },
      focusAmt: { value: 0 }, focusR: { value: 0.25 }, focusC: { value: new THREE.Vector2(0.5, 0.5) }, aspect: { value: 16 / 9 },
    },
    vertexShader: 'varying vec2 vUv; void main(){ vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }',
    fragmentShader: GRADE,
  });
  composer.addPass(grade);

  const W = {
    renderer, scene, camera, composer, grade: grade.uniforms, bloom, sun, hemi, skyMat, starMat,
    camKeys: [], jiaKeys: [], lightKeys: [], crowdKeys: [], warps: [], shopKeys: [],
    nightMats: [], lamps: [], updaters: [],
    designFov: 40,
  };

  buildCampus(W);
  buildShop(W);
  buildCrowd(W);
  buildJia(W);

  W.addCam = (keys) => W.camKeys.push(...keys);
  W.addJia = (keys) => W.jiaKeys.push(...keys);
  W.addLight = (keys) => W.lightKeys.push(...keys);
  W.addCrowd = (keys) => W.crowdKeys.push(...keys);
  // Crowd time runs at k× between a and b (k = 0 freezes it, k > 1 is a time-lapse).
  W.addWarp = (a, b, k) => W.warps.push([a, b, k]);
  W.onUpdate = (fn) => W.updaters.push(fn);
  // Shop state over time: { t, door: 'OPEN\n9–5', sign: 0..1 (faded → bright), lit: 0..1 (interior light) }.
  W.addShop = (keys) => W.shopKeys.push(...keys);

  W.finalize = () => {
    for (const k of [W.camKeys, W.jiaKeys, W.lightKeys, W.crowdKeys, W.shopKeys]) k.sort((a, b) => a.t - b.t);
  };

  W.resize = (w, h) => {
    renderer.setSize(w, h, false);
    composer.setSize(w, h);
    camera.aspect = w / h;
    grade.uniforms.aspect.value = w / h;
    camera.updateProjectionMatrix();
  };

  // Per-frame screen effects. Chapters adjust these; applyFx() hands them to the grade pass.
  W.fx = {};
  const resetFx = () => Object.assign(W.fx, { sat: 1, fade: 0, flash: 0, focus: null, focusR: 0.2, focusAmt: 0 });
  W.applyFx = () => {
    const g = grade.uniforms, f = W.fx;
    g.sat.value = f.sat; g.fade.value = f.fade; g.flash.value = f.flash;
    if (f.focus && f.focusAmt > 0.001) W.focus(f.focus, f.focusR, f.focusAmt);
    else g.focusAmt.value = 0;
  };

  W.update = (t) => {
    resetFx();
    updateLight(W, t);
    updateCamera(W, t);
    updateCrowd(W, t);
    updateJia(W, t);
    updateShop(W, t);
    for (const f of W.updaters) f(t);
  };

  W.render = () => composer.render();

  // Screen position (0..1, y down) of a world point, used by the grade's focus mask.
  const tmp = new THREE.Vector3();
  W.toScreen = (p) => {
    tmp.copy(p).project(camera);
    return { x: (tmp.x + 1) / 2, y: (1 - tmp.y) / 2, behind: tmp.z > 1 };
  };
  W.focus = (p, r, amt) => {
    const s = W.toScreen(p);
    grade.uniforms.focusC.value.set(s.x, 1 - s.y);
    grade.uniforms.focusR.value = r;
    grade.uniforms.focusAmt.value = amt;
  };

  return W;
}

// ---------------- light ----------------
const cA = new THREE.Color(), cB = new THREE.Color();
function mixPreset(a, b, f) {
  const o = {};
  for (const k of Object.keys(a)) {
    if (['sun', 'hemiS', 'hemiG', 'top', 'hor'].includes(k)) o[k] = cA.setHex(a[k]).lerp(cB.setHex(b[k]), f).getHex();
    else o[k] = lerp(a[k], b[k], f);
  }
  return o;
}

function presetAt(keys, t) {
  // keys: { t, p, d }: arrive at preset p by time t, blending over d seconds.
  let cur = PRESETS[keys[0].p];
  for (let i = 1; i < keys.length; i++) {
    const k = keys[i];
    if (t >= k.t) { cur = PRESETS[k.p]; continue; }
    if (t > k.t - k.d) return mixPreset(cur, PRESETS[k.p], smooth((t - (k.t - k.d)) / k.d));
    break;
  }
  return cur;
}

function updateLight(W, t) {
  const L = presetAt(W.lightKeys, t);
  const el = THREE.MathUtils.degToRad(L.el), az = THREE.MathUtils.degToRad(L.az);
  W.sun.position.set(Math.cos(el) * Math.sin(az) * 90, Math.sin(el) * 90, Math.cos(el) * Math.cos(az) * 90).add(W.sun.target.position);
  W.sun.color.setHex(L.sun);
  W.sun.intensity = L.sunI;
  W.hemi.color.setHex(L.hemiS);
  W.hemi.groundColor.setHex(L.hemiG);
  W.hemi.intensity = L.hemiI;
  W.skyMat.uniforms.top.value.setHex(L.top);
  W.skyMat.uniforms.hor.value.setHex(L.hor);
  W.scene.fog.color.setHex(L.hor);
  W.starMat.opacity = L.stars;
  W.renderer.toneMappingExposure = L.exp;
  W.night = L.night;
  for (const m of W.nightMats) m.emissiveIntensity = m.userData.nightI * L.night;
  for (const m of W.lamps) m.emissiveIntensity = 2.4 * clamp((L.night - 0.25) / 0.5);
}

// ---------------- camera ----------------
const pA = new THREE.Vector3(), lA = new THREE.Vector3(), pB = new THREE.Vector3(), lB = new THREE.Vector3();

function poseAt(k, t, outP, outL) {
  outP.fromArray(k.p);
  outL.fromArray(k.l);
  // A slow push-in while holding a shot, capped so it never becomes a zoom.
  const hold = Math.max(0, t - k.t);
  const push = Math.min(k.push ?? 0.035, hold * 0.004);
  outP.lerp(outL, push);
  return k.fov ?? 40;
}

function updateCamera(W, t) {
  const keys = W.camKeys;
  let i = 0;
  while (i + 1 < keys.length && t >= keys[i + 1].t - (keys[i + 1].d ?? 2)) i++;
  const k = keys[i];
  let fov = poseAt(k, t, pA, lA);
  // Moving toward key i (from key i-1) if we are inside its move window.
  if (i > 0 && t < k.t) {
    const prev = keys[i - 1];
    const start = k.t - (k.d ?? 2);
    const f = smoother((t - start) / (k.d ?? 2));
    const fovA = poseAt(prev, start, pB, lB);
    const fovB = poseAt(k, k.t, pA, lA);
    pA.lerpVectors(pB, pA, f);
    lA.lerpVectors(lB, lA, f);
    if (k.arc) pA.y += Math.sin(f * Math.PI) * k.arc;
    fov = lerp(fovA, fovB, f);
  }
  // Narrow screens: widen the view so the horizontal framing survives, then dolly back.
  const designAspect = 16 / 9;
  const cam = W.camera;
  if (cam.aspect < designAspect) {
    const hTan = Math.tan(THREE.MathUtils.degToRad(fov / 2)) * designAspect;
    let vTan = hTan / cam.aspect;
    const maxTan = Math.tan(THREE.MathUtils.degToRad(33));
    let back = 1;
    if (vTan > maxTan) { back = vTan / maxTan; vTan = maxTan; }
    fov = THREE.MathUtils.radToDeg(Math.atan(vTan)) * 2;
    pA.sub(lA).multiplyScalar(lerp(1, back, 0.85)).add(lA);
  }
  cam.position.copy(pA);
  cam.fov = fov;
  cam.updateProjectionMatrix();
  cam.lookAt(lA);
  cam.updateMatrixWorld();
}

// ---------------- campus ----------------
function windowGrid(W, { w, h, rows, cols, y0, face, depth, x0 = 0, z0 = 0, color = 0x5f7488, lit = 0xffd59a, nightI = 1.6, ww = 0.9, wh = 1.1 }) {
  // Windows on one facade: two instanced meshes, one that lights up at night and one that stays dark.
  const on = new THREE.MeshStandardMaterial({ color, roughness: 0.35, metalness: 0.1, emissive: lit, emissiveIntensity: 0 });
  on.userData.nightI = nightI;
  W.nightMats.push(on);
  const off = matS(color, 0.35, 0.1);
  const geo = new THREE.PlaneGeometry(ww, wh);
  const o = new THREE.Object3D();
  const r = rng(Math.round(w * 13 + h * 7 + rows + cols * 3));
  const lists = [[], []];
  for (let i = 0; i < rows; i++) for (let j = 0; j < cols; j++) {
    const u = (j + 0.5) / cols - 0.5;
    const y = y0 + i * (h / rows);
    if (face === 'S') o.position.set(x0 + u * w, y, z0 + depth / 2 + 0.02), o.rotation.set(0, 0, 0);
    if (face === 'N') o.position.set(x0 - u * w, y, z0 - depth / 2 - 0.02), o.rotation.set(0, Math.PI, 0);
    if (face === 'E') o.position.set(x0 + depth / 2 + 0.02, y, z0 - u * w), o.rotation.set(0, Math.PI / 2, 0);
    if (face === 'W') o.position.set(x0 - depth / 2 - 0.02, y, z0 + u * w), o.rotation.set(0, -Math.PI / 2, 0);
    o.updateMatrix();
    lists[r() < 0.72 ? 0 : 1].push(o.matrix.clone());
  }
  const g = new THREE.Group();
  [[lists[0], on], [lists[1], off]].forEach(([mats, m]) => {
    if (!mats.length) return;
    const inst = new THREE.InstancedMesh(geo, m, mats.length);
    mats.forEach((mx, k) => inst.setMatrixAt(k, mx));
    inst.receiveShadow = true;
    g.add(inst);
  });
  return g;
}

function building(W, { x, z, w, d, h, color, roof = PAL.roofGrey, faces = {}, floors = 2 }) {
  const g = new THREE.Group();
  g.add(place(rbox(w, h, d, color, 0.12), 0, h / 2, 0));
  g.add(place(rbox(w + 0.12, 0.45, d + 0.12, new THREE.Color(color).multiplyScalar(0.78).getHex(), 0.06), 0, 0.22, 0));
  g.add(place(rbox(w + 0.3, 0.22, d + 0.3, new THREE.Color(color).lerp(new THREE.Color(0xffffff), 0.35).getHex(), 0.05), 0, h - 0.05, 0));
  g.add(place(rbox(w + 0.4, 0.3, d + 0.4, roof, 0.08), 0, h + 0.15, 0));
  g.add(place(rbox(w * 0.18, 0.5, d * 0.22, 0x8a929c, 0.05), w * 0.22, h + 0.55, -d * 0.15));
  for (const [face, cols] of Object.entries(faces)) {
    const span = face === 'S' || face === 'N' ? w : d;
    g.add(windowGrid(W, { w: span * 0.86, h: h * 0.82, rows: floors, cols, y0: h * 0.3, face, depth: face === 'S' || face === 'N' ? d : w }));
  }
  g.position.set(x, 0, z);
  return g;
}

function pathSeg(a, b, w, material) {
  const dx = b[0] - a[0], dz = b[1] - a[1];
  const len = Math.hypot(dx, dz);
  const m = mesh(new THREE.BoxGeometry(len + w * 0.02, 0.04, w), material, { cast: false });
  m.position.set((a[0] + b[0]) / 2, 0.02, (a[1] + b[1]) / 2);
  m.rotation.y = -Math.atan2(dz, dx);
  return m;
}

function joint(p, w, material) {
  const m = mesh(new THREE.CylinderGeometry(w / 2, w / 2, 0.04, 20), material, { cast: false });
  m.position.set(p[0], 0.02, p[1]);
  return m;
}

function buildCampus(W) {
  const s = W.scene;
  const campus = new THREE.Group();
  W.campus = campus;
  s.add(campus);

  // Diorama base.
  campus.add(place(rbox(67, 3, 67, matS(PAL.plinth, 0.9), 0.6), 0, -1.6, -2));
  campus.add(place(rbox(65.4, 0.2, 65.4, matS(PAL.lawn, 0.95), 0.1), 0, -0.1, -2));

  const pm = matS(PAL.path, 0.95);
  const paths = [
    [[[0, 19], [0, -10.4]], 2.4],
    [[[-17.5, 2], [19.9, 2]], 2.2],
    [[[-9, 19], [13, 19]], 1.8],
    [[[-17.5, -0.2], [-17.5, 8]], 1.6],
    [[[-18.6, 0], [-17.5, 0]], 1.4],
    [[[-18.6, 7], [-17.5, 7]], 1.4],
    [[[-7, 19], [-7, 20.4]], 1.4],
    [[[11, 19], [11, 20.4]], 1.4],
  ];
  for (const [[a, b], w] of paths) campus.add(pathSeg(a, b, w, pm));
  for (const p of [[0, 19], [0, 2], [-17.5, 2], [-17.5, 7], [-17.5, 0]]) campus.add(joint(p, 2.4, pm));
  // Library plaza.
  campus.add(place(mesh(new THREE.BoxGeometry(12, 0.04, 3.2), pm, { cast: false }), 0, 0.02, -10.4));
  // The narrow side path to the shop.
  const sm = matS(0xd8cbb2, 0.95);
  for (let i = 0; i < SIDE_PATH.length - 1; i++) campus.add(pathSeg(SIDE_PATH[i], SIDE_PATH[i + 1], 1.3, sm));
  for (const p of SIDE_PATH.slice(1, -1)) campus.add(joint(p, 1.3, sm));
  // Old shop forecourt.
  campus.add(place(mesh(new THREE.BoxGeometry(9, 0.04, 2), pm, { cast: false }), 9, 0.02, 3.6));

  // Library: wide, columns at the front.
  const lib = building(W, { x: 0, z: -16, w: 18, d: 8, h: 7, color: PAL.stone, roof: 0x7f8a96, faces: { S: 9, E: 4, W: 4 }, floors: 2 });
  for (let i = 0; i < 6; i++) lib.add(place(mesh(new THREE.CylinderGeometry(0.28, 0.32, 5.6, 14), matS(0xeee4d0, 0.7)), -5 + i * 2, 2.8, 4.6));
  lib.add(place(rbox(12, 0.5, 2, PAL.stone, 0.06), 0, 5.85, 4.4));
  lib.add(place(rbox(11, 0.3, 1.6, 0xd2c4aa, 0.04), 0, 0.15, 4.9));
  W.libraryGroup = lib;
  campus.add(lib);

  campus.add(building(W, { x: -22.5, z: -3, w: 8, d: 9, h: 6, color: PAL.brick, faces: { E: 4, S: 3 }, floors: 3 }));
  campus.add(building(W, { x: -22.5, z: 9, w: 8, d: 8, h: 5, color: 0xc4a07c, faces: { E: 3, S: 3, N: 3 }, floors: 2 }));
  campus.add(building(W, { x: -7, z: 23, w: 12, d: 5, h: 5.2, color: 0xd9c3a5, roof: PAL.roof, faces: { N: 6, E: 2 }, floors: 3 }));
  campus.add(building(W, { x: 11, z: 23, w: 10, d: 5, h: 5.2, color: 0xcdb79a, roof: PAL.roof, faces: { N: 5, W: 2 }, floors: 3 }));
  campus.add(building(W, { x: 23.5, z: 3, w: 7, d: 9, h: 3.6, color: 0xe2d2b4, roof: 0x5f8a7a, faces: { W: 4, S: 3 }, floors: 1 }));

  // Student centre, whose left unit was the shop's old home (now empty).
  const sc = building(W, { x: 9, z: 6.5, w: 9, d: 5, h: 4.4, color: 0xd5c6ae, faces: { E: 2 }, floors: 2 });
  sc.add(place(rbox(3.6, 2.2, 0.1, mat(0x2c3440, { rough: 0.3 }), 0.02), -2.2, 1.4, -2.52));
  sc.add(place(rbox(3.6, 2.2, 0.1, mat(0x8fb4c8, { rough: 0.3, emissive: 0xffd59a, ei: 0 }), 0.02), 2.2, 1.4, -2.52));
  const toLet = sign({ w: 1.6, h: 0.45, text: 'TO LET', fg: '#6b6152', bg: '#efe6d4' });
  toLet.position.set(-2.2, 2.85, -2.56); toLet.rotation.y = Math.PI;
  sc.add(toLet);
  const cafeSign = sign({ w: 1.8, h: 0.45, text: 'PRINT · COPY', fg: '#2f3a44', bg: '#efe6d4' });
  cafeSign.position.set(2.2, 2.85, -2.56); cafeSign.rotation.y = Math.PI;
  sc.add(cafeSign);
  campus.add(sc);

  // Trees and lamp posts, placed deterministically away from buildings and paths.
  const r = rng(11);
  const blocked = [
    [-10, 10, -21, -9.5], [-27, -18, -8, 14], [-14, 18, 20, 26.5], [19.5, 28, -2.5, 8.5], [4, 14, 3, 9.5],
    [12.5, 22, -28.5, -16], [-1.6, 1.6, -11, 20], [-19, 21, 0.6, 3.4], [-10, 14, 17.8, 20.2], [3.8, 16, -11.5, -9], [13.6, 15.6, -14, -21],
    [-19, -16.5, -1, 8.5],
  ];
  const free = (x, z) => !blocked.some(([x0, x1, z0, z1]) => x > x0 - 1 && x < x1 + 1 && z > z0 - 1 && z < z1 + 1);
  let n = 0, guard = 0;
  while (n < 46 && guard++ < 2000) {
    const x = lerp(-31, 31, r()), z = lerp(-34, 29, r());
    if (!free(x, z)) continue;
    campus.add(place(tree(r, 0.8 + r() * 0.5), x, 0, z, r() * 6));
    n++;
  }
  for (const [x, z] of [[1.6, 14], [1.6, 7], [-1.6, -4], [6, 0.6], [-8, 0.6], [13, 3.4], [1.6, -8.8], [13.6, -14.6], [10.2, -12.2]]) {
    const lp = lampPost();
    lp.position.set(x, 0, z);
    W.lamps.push(lp.userData.bulb.material);
    campus.add(lp);
  }

  // Benches on the quad.
  for (const [x, z, ry] of [[-6, -1, 0], [-3.5, 5, Math.PI], [6, -4, Math.PI / 2]]) {
    const b = new THREE.Group();
    b.add(place(rbox(1.4, 0.08, 0.45, PAL.wood, 0.02), 0, 0.42, 0));
    b.add(place(rbox(1.4, 0.35, 0.06, PAL.wood, 0.02), 0, 0.65, -0.2));
    b.add(place(rbox(0.08, 0.42, 0.4, PAL.woodDark, 0.02), -0.6, 0.21, 0));
    b.add(place(rbox(0.08, 0.42, 0.4, PAL.woodDark, 0.02), 0.6, 0.21, 0));
    campus.add(place(b, x, 0, z, ry));
  }
}

// ---------------- the shop ----------------
function buildShop(W) {
  const g = new THREE.Group();
  g.position.copy(P.shop);
  const wall = matS(PAL.cream, 0.9);
  g.add(place(rbox(8, 0.12, 5.5, matS(0xc8a97e, 0.8), 0.03), 0, 0.06, 0));
  g.add(place(rbox(8, 3.2, 0.25, wall, 0.05), 0, 1.6, -2.62));
  g.add(place(rbox(0.25, 3.2, 5.5, wall, 0.05), -3.88, 1.6, 0));
  g.add(place(rbox(0.25, 3.2, 5.5, wall, 0.05), 3.88, 1.6, 0));
  // Front: door on the left, a long window to its right.
  g.add(place(rbox(5.7, 0.55, 0.25, wall, 0.04), 1.05, 0.28, 2.62));
  for (const x of [-3.3, -1.8, 3.88]) g.add(place(rbox(0.3, 3.2, 0.3, wall, 0.05), x, 1.6, 2.62));
  g.add(place(rbox(8.2, 0.7, 0.32, wall, 0.05), 0, 2.85, 2.62));
  g.add(place(rbox(8.7, 0.3, 6.2, matS(PAL.roof, 0.8), 0.08), 0, 3.35, 0));
  const glass = new THREE.MeshStandardMaterial({ color: PAL.glass, roughness: 0.12, metalness: 0.0, transparent: true, opacity: 0.22, emissive: 0xffc77a, emissiveIntensity: 0, depthWrite: false });
  const pane = new THREE.Mesh(new THREE.PlaneGeometry(5.4, 1.95), glass);
  pane.position.set(1.05, 1.53, 2.64);
  g.add(pane);

  const shopSign = sign({ w: 6.2, h: 0.55, text: 'SECOND-HAND BOOKS', fg: '#a39a8a', bg: '#e3d8c3' });
  shopSign.position.set(0.3, 2.85, 2.8);
  g.add(shopSign);

  const doorSign = sign({ w: 0.62, h: 0.42, text: 'OPEN\n9–5', fg: '#2f3a44', bg: '#fbf6ea', px: 160, size: 0.7 });
  doorSign.position.set(-1.8, 1.55, 2.79);
  g.add(doorSign);

  // Interior.
  g.add(place(shelf(2.2, 2.5, 0.5, { seed: 3, rows: 5 }), -2.4, 0.12, -2.2));
  g.add(place(shelf(2.2, 2.5, 0.5, { seed: 4, rows: 5 }), 0.0, 0.12, -2.2));
  g.add(place(shelf(2.2, 2.5, 0.5, { seed: 5, rows: 5 }), 2.4, 0.12, -2.2));
  const desk = counter(2.2);
  desk.position.set(2.2, 0.12, 0.2);
  g.add(desk);
  const till = rbox(0.4, 0.26, 0.3, 0x3a4250, 0.04);
  till.position.set(2.7, 1.2, 0.2);
  g.add(till);
  g.add(place(shelf(1.4, 1.0, 0.45, { seed: 6, rows: 2 }), -0.6, 0.12, 0.9, 0));

  const lamp = new THREE.PointLight(0xffd6a0, 0, 9, 1.6);
  lamp.position.set(0.5, 2.7, 0.2);
  g.add(lamp);

  W.scene.add(g);
  W.shop = { group: g, sign: shopSign, doorSign, glass, lamp };
}

function updateShop(W, t) {
  const st = { door: 'OPEN\n9–5', sign: 0, lit: 0.5 };
  let litFrom = 0.5, litAt = -1, signFrom = 0, signAt = -1;
  for (const k of W.shopKeys) {
    if (k.t > t) break;
    if (k.door !== undefined) st.door = k.door;
    if (k.lit !== undefined) { litFrom = st.lit; st.lit = k.lit; litAt = k.t; }
    if (k.sign !== undefined) { signFrom = st.sign; st.sign = k.sign; signAt = k.t; }
  }
  const lit = litAt < 0 ? st.lit : lerp(litFrom, st.lit, smooth((t - litAt) / 0.8));
  const sg = signAt < 0 ? st.sign : lerp(signFrom, st.sign, smooth((t - signAt) / 1.2));
  const S = W.shop;
  S.doorSign.userData.set(st.door);
  S.lamp.intensity = lit * 7;
  S.glass.emissiveIntensity = lit * (0.15 + 0.6 * (W.night ?? 0));
  const q = Math.round(sg * 4) / 4;
  S.sign.userData.set(null, q < 0.5
    ? { fg: '#a39a8a', bg: '#e3d8c3' }
    : { fg: '#fff6e0', bg: q < 1 ? '#3f7a64' : '#2f6b55' });
  S.sign.material.emissive?.setHex(0x000000);
}

// ---------------- crowd ----------------
const ROUTES = [
  [[-7, 20.4], [-7, 19], [0, 19], [0, 2], [-17.5, 2], [-17.5, 0], [-18.6, 0]],
  [[11, 20.4], [11, 19], [0, 19], [0, -10.6]],
  [[-18.6, 7], [-17.5, 7], [-17.5, 2], [19.9, 2]],
  [[0, -10.6], [0, 2], [19.9, 2]],
  [[-7, 20.4], [-7, 19], [0, 19], [0, 2], [19.9, 2]],
  [[-18.6, 0], [-17.5, 0], [-17.5, 2], [0, 2], [0, -10.6]],
];

function buildCrowd(W) {
  const N = 52;
  const bodyMat = new THREE.MeshStandardMaterial({ roughness: 0.8 });
  const headMat = new THREE.MeshStandardMaterial({ roughness: 0.7 });
  const bodies = new THREE.InstancedMesh(FIG.bodyGeo, bodyMat, N);
  const heads = new THREE.InstancedMesh(FIG.headGeo, headMat, N);
  bodies.castShadow = heads.castShadow = true;
  bodies.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
  heads.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
  bodies.frustumCulled = heads.frustumCulled = false;
  const curves = ROUTES.map((r) => polyPath(r));
  const lens = curves.map((c) => c.getLength());
  const r = rng(5);
  const walkers = [];
  for (let i = 0; i < N; i++) {
    const route = i % ROUTES.length;
    walkers.push({ route, phase: r(), speed: 1.05 + r() * 0.45, side: (r() - 0.5) * 1.3, gate: r(), bob: r() * 10 });
    bodies.setColorAt(i, new THREE.Color(pick(r, PAL.shirts)));
    heads.setColorAt(i, new THREE.Color(pick(r, PAL.skin)));
  }
  W.scene.add(bodies, heads);
  W.crowd = { bodies, heads, walkers, curves, lens };
}

const o3 = new THREE.Object3D();
const tv = new THREE.Vector3(), nv = new THREE.Vector3();

function crowdClock(W, t) {
  let extra = 0;
  for (const [a, b, k] of W.warps) if (t > a) extra += (Math.min(t, b) - a) * (k - 1);
  return t + extra;
}

function levelAt(keys, t) {
  if (!keys.length) return 1;
  let cur = keys[0].v;
  for (let i = 1; i < keys.length; i++) {
    const k = keys[i];
    if (t >= k.t) { cur = k.v; continue; }
    if (t > k.t - k.d) return lerp(cur, k.v, smooth((t - (k.t - k.d)) / k.d));
    break;
  }
  return cur;
}

function updateCrowd(W, t) {
  const { bodies, heads, walkers, curves, lens } = W.crowd;
  const tau = crowdClock(W, t);
  const level = levelAt(W.crowdKeys, t);
  walkers.forEach((w, i) => {
    const curve = curves[w.route];
    const u = tau * w.speed / lens[w.route] + w.phase;
    const k = Math.floor(u), f = u - k;
    const fwd = k % 2 === 0;
    const at = fwd ? f : 1 - f;
    curve.getPointAt(at, tv);
    curve.getTangentAt(at, nv);
    if (!fwd) nv.negate();
    const yaw = Math.atan2(nv.x, nv.z);
    // Keep to one side of the path, like people do.
    tv.x += nv.z * w.side * 0.5 * (fwd ? 1 : 1);
    tv.z -= nv.x * w.side * 0.5;
    let s = smooth(f / 0.025) * smooth((1 - f) / 0.025);
    s *= clamp((level - w.gate) * 8 + 0.5);
    const bob = Math.abs(Math.sin(tau * 7 + w.bob)) * 0.05;
    o3.position.set(tv.x, FIG.bodyY + bob, tv.z);
    o3.rotation.set(0, yaw, Math.sin(tau * 7 + w.bob) * 0.05);
    o3.scale.setScalar(Math.max(s, 0.0001));
    o3.updateMatrix();
    bodies.setMatrixAt(i, o3.matrix);
    o3.position.y = FIG.headY + bob;
    o3.rotation.set(0, yaw, 0);
    o3.updateMatrix();
    heads.setMatrixAt(i, o3.matrix);
  });
  bodies.instanceMatrix.needsUpdate = true;
  heads.instanceMatrix.needsUpdate = true;
}

// ---------------- Jia ----------------
function buildJia(W) {
  const jia = figure({ shirt: 0xe9e2d0, skin: 0xe8c4a4, apron: true, notebook: true, hair: 0x2a2220 });
  W.scene.add(jia);
  W.jia = jia;
}

function angleLerp(a, b, f) {
  let d = ((b - a + Math.PI) % (Math.PI * 2) + Math.PI * 2) % (Math.PI * 2) - Math.PI;
  return a + d * f;
}

function updateJia(W, t) {
  const keys = W.jiaKeys, jia = W.jia;
  if (!keys.length) return;
  let i = 0;
  while (i + 1 < keys.length && t >= keys[i + 1].t) i++;
  const k = keys[i], n = keys[i + 1];
  if (k.hide) { jia.visible = false; return; }
  jia.visible = true;
  const moving = n && !n.hide && (n.x !== k.x || n.z !== k.z) && n.walk !== false;
  let x = k.x, z = k.z, y = k.y ?? 0, yaw = k.ry ?? 0, walking = 0, phase = 0;
  if (moving) {
    // Walk from k to n, starting so that we arrive exactly at n.t.
    const dist = Math.hypot(n.x - k.x, n.z - k.z);
    const start = Math.max(k.t, n.t - (n.dur ?? Math.max(0.8, dist / 1.5)));
    const dur = n.t - start;
    if (t > start) {
      const f = clamp((t - start) / dur);
      x = lerp(k.x, n.x, f); z = lerp(k.z, n.z, f); y = lerp(k.y ?? 0, n.y ?? 0, f);
      const moveYaw = Math.atan2(n.x - k.x, n.z - k.z);
      yaw = angleLerp(k.ry ?? 0, moveYaw, clamp((t - start) / 0.35));
      walking = Math.min(1, f * 8, (1 - f) * 8);
      phase = f * dist * 4.2;
    }
  } else if (i > 0) {
    // Just arrived: turn toward this key's facing.
    const p = keys[i - 1];
    if (!p.hide && (p.x !== k.x || p.z !== k.z)) {
      const moveYaw = Math.atan2(k.x - p.x, k.z - p.z);
      yaw = angleLerp(moveYaw, k.ry ?? 0, clamp((t - k.t) / 0.5));
    }
  }
  jia.position.set(x, y, z);
  jia.rotation.y = yaw;
  pose(jia, { walking, phase, look: k.look ?? 0, nod: k.nod ?? 0 });
}
