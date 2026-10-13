#version 450
// One direction of a blur that keeps to surfaces at a similar depth, so occlusion doesn't bleed
// across silhouettes.
layout(set = 0, binding = 0) uniform sampler2D src;  // r = visibility, g = distance
layout(push_constant) uniform P { vec2 step; } pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

void main() {
  vec4 c = texture(src, vUv);
  float sum = c.r, wsum = 1.0;
  float tol = 0.03 * c.g + 0.04;
  for (int i = -4; i <= 4; i++) {
    if (i == 0) continue;
    vec4 s = texture(src, vUv + pc.step * float(i));
    float k = abs(s.g - c.g) / tol;
    float w = exp(-float(i * i) / 12.0) / (1.0 + k * k);
    sum += s.r * w;
    wsum += w;
  }
  outColor = vec4(sum / wsum, c.g, 0.0, 1.0);
}
