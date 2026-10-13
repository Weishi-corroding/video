#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNrm;
layout(location = 2) in vec2 aUv;

layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUv;
layout(location = 3) out vec3 vColor;
layout(location = 4) out float vDepth;

void main() {
  Draw d = draws[pc.draw];
  vec4 wp = d.model * vec4(aPos, 1.0);
  vWorld = wp.xyz;
  vNormal = vec3(0.0, 1.0, 0.0);
  vUv = vec2(0.5);
  vColor = vec3(1.0);
  vDepth = -(F.view * wp).z;
  gl_Position = F.viewProj * wp;
  gl_PointSize = max(1.0, d.params.z * F.viewport.z);
}
