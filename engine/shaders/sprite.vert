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

// A quad that always faces the camera, sized by the node's scale.
void main() {
  Draw d = draws[pc.draw];
  vec3 c = d.model[3].xyz;
  float sx = length(d.model[0].xyz), sy = length(d.model[1].xyz);
  vec4 wp = vec4(c + F.camRight.xyz * aPos.x * sx + F.camUp.xyz * aPos.y * sy, 1.0);
  vWorld = wp.xyz;
  vNormal = vec3(0.0, 0.0, 1.0);
  vUv = aUv;
  vColor = vec3(1.0);
  vDepth = -(F.view * wp).z;
  gl_Position = F.viewProj * wp;
}
