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
// Object space at world scale, for patterns that stay put on moving objects.
layout(location = 5) out vec3 vObj;
layout(location = 6) out vec3 vObjN;

void main() {
  Draw d = draws[pc.draw];
  vec3 p = aPos, n = aNrm, col = vec3(1.0);
  vec3 scale = vec3(length(d.model[0].xyz), length(d.model[1].xyz), length(d.model[2].xyz));
  if ((d.info.x & F_INSTANCED) != 0u) {
    mat4 im = inst[d.info.z + gl_InstanceIndex];
    scale *= vec3(length(im[0].xyz), length(im[1].xyz), length(im[2].xyz));
    // Scale the normal back before the instance's rotation-and-scale (no shear), as an inverse-transpose would.
    vec3 s2 = vec3(dot(im[0].xyz, im[0].xyz), dot(im[1].xyz, im[1].xyz), dot(im[2].xyz, im[2].xyz));
    n = mat3(im) * (n / max(s2, vec3(1e-12)));
    p = (im * vec4(p, 1.0)).xyz;
    if ((d.info.x & F_ICOLOR) != 0u) col = icol[d.info.z + gl_InstanceIndex].rgb;
  }
  vObj = aPos * scale;
  vObjN = aNrm;
  vec4 wp = d.model * vec4(p, 1.0);
  vWorld = wp.xyz;
  vNormal = mat3(d.n0.xyz, d.n1.xyz, d.n2.xyz) * n;
  vUv = aUv;
  vColor = col;
  vDepth = -(F.view * wp).z;
  gl_Position = F.viewProj * wp;
}
