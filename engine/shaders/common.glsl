// Shared declarations for the scene shaders.
#define PI 3.141592653589793

#define F_LIT 1u
#define F_MAP 2u
#define F_INSTANCED 4u
#define F_ICOLOR 8u
#define F_RECEIVE 16u
#define F_FOG 32u
#define F_DOUBLE 64u

layout(set = 0, binding = 0, std140) uniform FrameU {
  mat4 viewProj;
  mat4 view;
  mat4 invViewProj;
  mat4 shadowViewProj;
  mat4 shadowMat;
  vec4 camPos;
  vec4 sunDir;       // xyz toward the sun, w = 1 when the sun casts shadows
  vec4 sunColor;     // colour * intensity
  vec4 fillDir;      // xyz toward the fill light
  vec4 fillColor;    // colour * intensity
  vec4 hemiSky;      // colour * intensity
  vec4 hemiGround;
  vec4 fogColor;     // w = 1 when fog is on
  vec4 fogRange;     // near, far
  vec4 shadowParams; // bias, normal bias, 1 / map size, filter radius in texels
  vec4 camRight;
  vec4 camUp;
  vec4 viewport;     // width, height, point scale, point light count
  vec4 skyTop;       // w = sky radius
  vec4 skyHorizon;
  vec4 pointPos[4];
  vec4 pointColor[4];
  vec4 pointParams[4]; // distance, decay
} F;

struct Draw {
  mat4 model;
  vec4 n0, n1, n2;   // normal matrix columns
  vec4 color;        // rgb, a = opacity
  vec4 emissive;     // rgb * intensity
  vec4 params;       // roughness, metalness, point size
  uvec4 info;        // flags, texture slot, first instance
};

layout(set = 0, binding = 1, std430) readonly buffer Draws { Draw draws[]; };
layout(set = 0, binding = 2, std430) readonly buffer Instances { mat4 inst[]; };
layout(set = 0, binding = 3, std430) readonly buffer InstanceColors { vec4 icol[]; };

layout(push_constant) uniform PC { uint draw; } pc;
