#version 450
layout(set = 0, binding = 0) uniform sampler2D src;
layout(push_constant) uniform P { float threshold; } pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;
// Keep only pixels brighter than the threshold.
void main() {
  vec4 c = texture(src, vUv);
  float l = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
  outColor = mix(vec4(0.0), c, smoothstep(pc.threshold, pc.threshold + 0.01, l));
}
