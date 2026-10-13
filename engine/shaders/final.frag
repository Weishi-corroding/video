#version 450
layout(set = 0, binding = 0) uniform sampler2D scene;
layout(set = 0, binding = 1) uniform sampler2D bloom[5];
layout(push_constant) uniform P {
  vec4 bloomFactors0;  // levels 0-3, already scaled by strength
  vec4 misc;           // level 4 factor, exposure, aspect, dither
  vec4 grade;          // saturation, vignette, fade, flash
  vec4 focus;          // centre x, centre y (0..1, y down), radius, amount
} pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

// Filmic tone curve: the ACES fit by Stephen Hill.
vec3 rrtOdt(vec3 v) {
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return a / b;
}
vec3 aces(vec3 c) {
  const mat3 inM = mat3(vec3(0.59719, 0.07600, 0.02840), vec3(0.35458, 0.90834, 0.13383), vec3(0.04823, 0.01566, 0.83777));
  const mat3 outM = mat3(vec3(1.60475, -0.10208, -0.00327), vec3(-0.53108, 1.10813, -0.07276), vec3(-0.07367, -0.00605, 1.07602));
  c = inM * (c / 0.6);
  c = rrtOdt(c);
  return clamp(outM * c, 0.0, 1.0);
}
vec3 encodeSRGB(vec3 c) {
  return mix(c * 12.92, pow(c, vec3(0.41666)) * 1.055 - 0.055, step(vec3(0.0031308), c));
}
float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }

void main() {
  vec3 c = texture(scene, vUv).rgb;
  c += pc.bloomFactors0.x * texture(bloom[0], vUv).rgb + pc.bloomFactors0.y * texture(bloom[1], vUv).rgb +
       pc.bloomFactors0.z * texture(bloom[2], vUv).rgb + pc.bloomFactors0.w * texture(bloom[3], vUv).rgb +
       pc.misc.x * texture(bloom[4], vUv).rgb;
  c = encodeSRGB(aces(c * pc.misc.y));

  // Grade: saturation, a focus mask that dims and greys everything outside a circle, vignette, flash, fade.
  float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
  vec2 d = (vUv - pc.focus.xy) * vec2(pc.misc.z, 1.0);
  float outside = smoothstep(pc.focus.z, pc.focus.z * 1.35 + 0.02, length(d)) * pc.focus.w;
  c = mix(vec3(l), c, pc.grade.x * (1.0 - 0.85 * outside));
  c *= 1.0 - 0.72 * outside;
  vec2 v = vUv - 0.5;
  c *= 1.0 - pc.grade.y * dot(v, v) * 1.5;
  c = mix(c, vec3(1.0), pc.grade.w);
  c *= 1.0 - pc.grade.z;
  c += (hash(gl_FragCoord.xy) - 0.5) / 255.0 * pc.misc.w;
  outColor = vec4(c, 1.0);
}
