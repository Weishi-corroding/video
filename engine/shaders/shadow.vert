#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(location = 0) in vec3 aPos;

void main() {
  Draw d = draws[pc.draw];
  vec3 p = aPos;
  if ((d.info.x & F_INSTANCED) != 0u) p = (inst[d.info.z + gl_InstanceIndex] * vec4(p, 1.0)).xyz;
  gl_Position = F.shadowViewProj * (d.model * vec4(p, 1.0));
}
