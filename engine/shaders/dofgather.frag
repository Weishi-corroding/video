#version 450
// Depth of field, step 2 (half resolution): gather a disc of samples whose circles reach this pixel.
// Background and foreground are gathered apart, so a blurred foreground can spill over sharp things
// behind it but a blurred background cannot spill over sharp things in front.
layout(set = 0, binding = 0) uniform sampler2D src;  // rgb, a = signed circle of confusion
layout(push_constant) uniform P { vec2 tex; float maxR; } pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;  // rgb, a = how much the foreground covers this pixel

float weigh(float coc, float r) { return clamp((coc - r + 2.0) / 2.0, 0.0, 1.0); }

void main() {
  vec4 center = texture(src, vUv);
  vec3 bg = vec3(0.0), fg = vec3(0.0);
  float bgW = 0.0, fgW = 0.0;
  int count = 0;
  for (int ring = 0; ring <= 3; ring++) {
    int n = ring == 0 ? 1 : ring * 6;
    float rr = float(ring) / 3.0;
    for (int k = 0; k < n; k++) {
      float a = (float(k) + 0.5 * float(ring & 1)) / float(n) * 6.2831853;
      vec2 o = vec2(cos(a), sin(a)) * rr * pc.maxR;
      vec4 s = texture(src, vUv + o * pc.tex);
      float r = length(o);
      float bw = weigh(max(0.0, min(s.a, center.a)), r);
      float fw = weigh(-s.a, r);
      bg += s.rgb * bw;
      bgW += bw;
      fg += s.rgb * fw;
      fgW += fw;
      count++;
    }
  }
  bg = bgW > 1e-4 ? bg / bgW : center.rgb;
  fg = fgW > 1e-4 ? fg / fgW : center.rgb;
  float cover = min(1.0, fgW * 3.14159265 / float(count));
  outColor = vec4(mix(bg, fg, cover), cover);
}
