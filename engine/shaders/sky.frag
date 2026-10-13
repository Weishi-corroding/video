#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 outColor;

// A gradient painted on a sphere around the origin: find where the view ray meets it.
void main() {
  vec4 far = F.invViewProj * vec4(vNdc, 1.0, 1.0);
  vec3 o = F.camPos.xyz;
  vec3 dir = normalize(far.xyz / far.w - o);
  float R = F.skyTop.w;
  float b = dot(o, dir), c = dot(o, o) - R * R;
  float t = -b + sqrt(max(b * b - c, 0.0));
  vec3 p = normalize(o + dir * t);
  float h = clamp(p.y * 1.6 + 0.25, 0.0, 1.0);
  outColor = vec4(mix(F.skyHorizon.rgb, F.skyTop.rgb, h), 1.0);
}
