#version 450
// Depth of field, step 1 (half resolution): the colour, with ambient occlusion applied, and the
// circle of confusion (signed, in half-resolution pixels: negative in front of the focus).
layout(set = 0, binding = 0) uniform sampler2D hdr;
layout(set = 0, binding = 2) uniform sampler2D aux[4];  // 0: ambient occlusion, 1: normal and depth
layout(push_constant) uniform P {
  vec4 p;  // focus distance, aperture, largest radius, occlusion strength
  vec4 q;  // 1 / width, 1 / height of the full-resolution buffers
} pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

// The sky is a painted backdrop and stays sharp, so stars stay points (clouds are geometry and blur).
float coc(float d) { return d > 50000.0 ? 0.0 : clamp(pc.p.y * (d - pc.p.x) / d, -pc.p.z, pc.p.z); }

void main() {
  vec3 c = texture(hdr, vUv).rgb;
  if (pc.p.w > 0.0) {
    float ao = texture(aux[0], vUv).r;
    ao = mix(ao, 1.0, smoothstep(1.5, 4.0, dot(c, vec3(0.2126, 0.7152, 0.0722))));
    c *= mix(1.0, ao, pc.p.w);
  }
  // The strongest circle among the four full-resolution pixels under this one.
  vec2 o = pc.q.xy * 0.5;
  float c0 = coc(texture(aux[1], vUv + vec2(-o.x, -o.y)).w), c1 = coc(texture(aux[1], vUv + vec2(o.x, -o.y)).w);
  float c2 = coc(texture(aux[1], vUv + vec2(-o.x, o.y)).w), c3 = coc(texture(aux[1], vUv + vec2(o.x, o.y)).w);
  float lo = min(min(c0, c1), min(c2, c3)), hi = max(max(c0, c1), max(c2, c3));
  outColor = vec4(c, hi >= -lo ? hi : lo);
}
