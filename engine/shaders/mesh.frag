#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(set = 0, binding = 4) uniform sampler2DShadow shadowMap;
layout(set = 0, binding = 5) uniform sampler2D textures[128];
layout(set = 0, binding = 6) uniform sampler2D paperTex;

layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUv;
layout(location = 3) in vec3 vColor;
layout(location = 4) in float vDepth;
layout(location = 5) in vec3 vObj;
layout(location = 6) in vec3 vObjN;

layout(location = 0) out vec4 outColor;
// View-space normal and distance along the view, for ambient occlusion and depth of field.
layout(location = 1) out vec4 outAux;

// Diffuse (Lambert, optionally wrapped) plus specular (GGX distribution, height-correlated Smith
// visibility, Schlick Fresnel).
vec3 shade(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 diffuse, vec3 f0, float rough, float wrap) {
  float nl = dot(N, L);
  float NLd = clamp((nl + wrap) / (1.0 + wrap), 0.0, 1.0);
  float NL = clamp(nl, 0.0, 1.0);
  if (NLd <= 0.0) return vec3(0.0);
  vec3 spec = vec3(0.0);
  if (NL > 0.0) {
    vec3 H = normalize(L + V);
    float NV = clamp(dot(N, V), 0.0, 1.0), NH = clamp(dot(N, H), 0.0, 1.0), VH = clamp(dot(V, H), 0.0, 1.0);
    float a = rough * rough, a2 = a * a;
    float gv = NL * sqrt(a2 + (1.0 - a2) * NV * NV);
    float gl = NV * sqrt(a2 + (1.0 - a2) * NL * NL);
    float vis = 0.5 / max(gv + gl, 1e-6);
    float dn = NH * NH * (a2 - 1.0) + 1.0;
    float D = a2 / (PI * dn * dn);
    float fr = exp2((-5.55473 * VH - 6.98316) * VH);
    vec3 Fr = f0 * (1.0 - fr) + vec3(fr);
    spec = Fr * (vis * D) * NL;
  }
  return radiance * (NLd * diffuse / PI + spec);
}

float sunShadow(vec3 wp, vec3 n) {
  vec4 sc = F.shadowMat * vec4(wp + n * F.shadowParams.y, 1.0);
  vec3 c = sc.xyz / sc.w;
  c.z += F.shadowParams.x;
  if (c.x < 0.0 || c.x > 1.0 || c.y < 0.0 || c.y > 1.0 || c.z > 1.0) return 1.0;
  // Sixteen filtered comparisons on a rotated disc over the filter radius.
  float r = F.shadowParams.z * F.shadowParams.w;
  float ang = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715)))) * 6.2831853;
  mat2 rot = mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
  float sum = 0.0;
  for (int i = 0; i < 16; i++) {
    float fi = (float(i) + 0.5) / 16.0;
    float a = float(i) * 2.39996323;
    vec2 o = rot * vec2(cos(a), sin(a)) * sqrt(fi) * r;
    sum += texture(shadowMap, vec3(c.xy + o, c.z));
  }
  return sum / 16.0;
}

// The paper sheet mapped onto the surface from three sides, blended by the object-space normal.
vec4 paperSample(vec3 p, vec3 n) {
  vec3 w = abs(normalize(n + vec3(1e-5)));
  w = w * w * w * w;
  w /= dot(w, vec3(1.0));
  p /= F.paper.x;
  return texture(paperTex, p.zy) * w.x + texture(paperTex, p.xz) * w.y + texture(paperTex, p.xy) * w.z;
}

void main() {
  Draw d = draws[pc.draw];
  uint f = d.info.x;
  vec4 base = vec4(d.color.rgb * vColor, d.color.a);
  if ((f & F_MAP) != 0u) base *= texture(textures[d.info.y], vUv);
  vec3 N = normalize(length(vNormal) > 1e-6 ? vNormal : vec3(0.0, 0.0, 1.0));
  if ((f & F_DOUBLE) != 0u && !gl_FrontFacing) N = -N;
  vec3 col;
  if ((f & F_LIT) != 0u) {
    vec3 V = normalize(F.camPos.xyz - vWorld);
    float paper = d.params.w;
    float wrap = 0.0;
    if (paper > 0.0) {
      vec4 t = paperSample(vObj, vObjN);
      base.rgb *= 1.0 + (t.r - 0.5) * 2.0 * F.paper.z * paper;
      // Relief from the fibre height, as a bump on the interpolated normal (surface gradient method).
      float h = t.g * F.paper.y * 0.01 * paper;
      vec3 dpdx = dFdx(vWorld), dpdy = dFdy(vWorld);
      vec3 r1 = cross(dpdy, N), r2 = cross(N, dpdx);
      float det = dot(dpdx, r1);
      vec3 grad = sign(det) * (dFdx(h) * r1 + dFdy(h) * r2);
      vec3 nb = abs(det) * N - grad;
      if (abs(det) > 1e-12 && dot(nb, nb) > 1e-24) N = normalize(nb);
      wrap = F.paper.w * paper;
    }
    float metal = d.params.y;
    vec3 diffuse = base.rgb * (1.0 - metal);
    vec3 f0 = mix(vec3(0.04), base.rgb, metal);
    // Roughen where the surface normal changes quickly across a pixel, to keep highlights from aliasing.
    vec3 dxy = max(abs(dFdx(N)), abs(dFdy(N)));
    float rough = min(max(d.params.x, 0.0525) + max(max(dxy.x, dxy.y), dxy.z), 1.0);

    float sh = 1.0;
    if (F.sunDir.w > 0.5 && (f & F_RECEIVE) != 0u) sh = sunShadow(vWorld, N);
    vec3 light = shade(normalize(F.sunDir.xyz), F.sunColor.rgb * sh, N, V, diffuse, f0, rough, wrap);
    if (F.fillColor.r + F.fillColor.g + F.fillColor.b > 0.0) light += shade(normalize(F.fillDir.xyz), F.fillColor.rgb, N, V, diffuse, f0, rough, wrap);
    int np = int(F.viewport.w);
    for (int i = 0; i < np; i++) {
      vec3 L = F.pointPos[i].xyz - vWorld;
      float dist = length(L);
      float att = 1.0 / max(pow(dist, F.pointParams[i].y), 0.01);
      if (F.pointParams[i].x > 0.0) {
        float k = clamp(1.0 - pow(dist / F.pointParams[i].x, 4.0), 0.0, 1.0);
        att *= k * k;
      }
      light += shade(L / max(dist, 1e-4), F.pointColor[i].rgb * att, N, V, diffuse, f0, rough, wrap);
    }
    vec3 irradiance = mix(F.hemiGround.rgb, F.hemiSky.rgb, 0.5 * N.y + 0.5);
    col = light + irradiance * diffuse / PI + d.emissive.rgb;
  } else {
    col = base.rgb;
  }
  if ((f & F_FOG) != 0u && F.fogColor.w > 0.5) col = mix(col, F.fogColor.rgb, smoothstep(F.fogRange.x, F.fogRange.y, vDepth));
  outColor = vec4(col, base.a);
  outAux = vec4(normalize(mat3(F.view) * N), vDepth);
}
