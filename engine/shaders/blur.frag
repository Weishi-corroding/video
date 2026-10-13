#version 450
layout(set = 0, binding = 0) uniform sampler2D src;
layout(push_constant) uniform P { vec2 step; int radius; float sigma; } pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

float gauss(float x, float s) { return 0.39894 * exp(-0.5 * x * x / (s * s)) / s; }

// One direction of a separable Gaussian blur.
void main() {
  float w0 = gauss(0.0, pc.sigma);
  vec3 sum = texture(src, vUv).rgb * w0;
  float wsum = w0;
  for (int i = 1; i < pc.radius; i++) {
    float w = gauss(float(i), pc.sigma);
    vec2 o = pc.step * float(i);
    sum += (texture(src, vUv + o).rgb + texture(src, vUv - o).rgb) * w;
    wsum += 2.0 * w;
  }
  outColor = vec4(sum / wsum, 1.0);
}
