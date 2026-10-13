#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(set = 0, binding = 6) uniform sampler2D paperTex;

layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outAux;

// A gradient painted on a sphere around the origin: find where the view ray meets it. With
// paperSky > 0 the paint shows the fibres of the sheet underneath, like a painted backdrop.
void main() {
  vec4 far = F.invViewProj * vec4(vNdc, 1.0, 1.0);
  vec3 o = F.camPos.xyz;
  vec3 dir = normalize(far.xyz / far.w - o);
  float R = F.skyTop.w;
  float b = dot(o, dir), c = dot(o, o) - R * R;
  float t = -b + sqrt(max(b * b - c, 0.0));
  vec3 p = normalize(o + dir * t);
  float h = clamp(p.y * 2.6 + 0.32, 0.0, 1.0);
  vec3 col = mix(F.skyHorizon.rgb, F.skyTop.rgb, h);
  if (F.paperSky.x > 0.0) {
    // Triplanar on the view direction, so there is no seam anywhere in the sky.
    vec3 w = abs(p);
    w = w * w * w * w;
    w /= dot(w, vec3(1.0));
    vec3 q = p * 7.0;
    vec4 tx = texture(paperTex, q.zy) * w.x + texture(paperTex, q.xz) * w.y + texture(paperTex, q.xy) * w.z;
    col *= 1.0 + (tx.r - 0.5) * 2.0 * F.paperSky.x;
  }
  outColor = vec4(col, 1.0);
  outAux = vec4(0.0, 0.0, 1.0, 60000.0);
}
