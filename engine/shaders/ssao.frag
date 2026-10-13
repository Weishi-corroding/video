#version 450
// Ambient occlusion from the normal-and-depth buffer: a rotated spiral of samples in the hemisphere
// above each surface, each counted when something nearby in the buffer covers it.
layout(set = 0, binding = 0) uniform sampler2D gbuf;  // view-space normal, distance along the view (unfiltered)
layout(push_constant) uniform P {
  vec4 proj;    // P[0][0], P[1][1], 1 / width, 1 / height
  vec4 params;  // radius, bias, power
} pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outAo;  // r = visibility, g = distance (for the blur)

vec3 viewPos(vec2 uv, float d) {
  vec2 ndc = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
  return vec3(ndc.x * d / pc.proj.x, ndc.y * d / pc.proj.y, -d);
}

void main() {
  vec4 g = texture(gbuf, vUv);
  float d = g.w;
  if (d > 50000.0) {
    outAo = vec4(1.0, d, 0.0, 1.0);
    return;
  }
  vec3 P = viewPos(vUv, d);
  // Multisample resolve can average opposite normals (two faces meeting in one pixel) toward zero.
  vec3 N = dot(g.xyz, g.xyz) > 1e-4 ? normalize(g.xyz) : vec3(0.0, 0.0, 1.0);
  // Interleaved gradient noise turns the pattern per pixel; the blur afterwards evens it out.
  float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
  vec3 helper = abs(N.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 T = normalize(cross(helper, N));
  vec3 B = cross(N, T);
  float R = pc.params.x * (1.0 + d * 0.012);
  float bias = pc.params.y + d * 0.0015;
  const int NS = 14;
  float occ = 0.0;
  for (int i = 0; i < NS; i++) {
    float fi = (float(i) + 0.5) / float(NS);
    float phi = float(i) * 2.39996323 + noise * 6.2831853;
    float r = sqrt(fi);
    vec3 h = vec3(cos(phi) * r, sin(phi) * r, sqrt(max(0.0, 1.0 - fi)));
    float len = 0.15 + 0.85 * pow(fract(fi * 3.0 + noise * 0.618), 2.0);
    vec3 S = P + (T * h.x + B * h.y + N * h.z) * R * len;
    if (S.z > -0.05) continue;
    vec2 suv = vec2(pc.proj.x * S.x / -S.z * 0.5 + 0.5, 0.5 - pc.proj.y * S.y / -S.z * 0.5);
    if (suv.x < 0.0 || suv.y < 0.0 || suv.x > 1.0 || suv.y > 1.0) continue;
    float sd = texture(gbuf, suv).w;
    float range = smoothstep(0.0, 1.0, R / max(abs(d - sd), 1e-4));
    occ += step(sd, -S.z - bias) * range;
  }
  float ao = clamp(1.0 - occ / float(NS), 0.0, 1.0);
  ao = exp2(pc.params.z * log2(max(ao, 1e-4)));
  outAo = vec4(isnan(ao) || isinf(ao) ? 1.0 : ao, d, 0.0, 1.0);
}
