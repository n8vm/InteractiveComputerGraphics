#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_multiview : enable
precision highp float;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 w_normal;
layout(location = 1) out vec3 w_position;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 w_reflection;

#define MAX_MULTIVIEW 6
struct PerspectiveObject {
    mat4 view;
    mat4 proj;
    mat4 viewinv;
    mat4 projinv;
    float near;
    float far;
    float pad1, pad2;
};

layout(binding = 0) uniform PerspectiveBufferObject {
    PerspectiveObject at[MAX_MULTIVIEW];
} pbo;


layout(binding = 1) uniform TransformBufferObject{
    mat4 worldToLocal;
    mat4 localToWorld;
} tbo;

#define MAXLIGHTS 10
struct PointLightBufferItem {
  vec4 color;
  int falloffType;
  float intensity;
  int pad2;
  int pad3;

  /* For shadow mapping */
  mat4 localToWorld;
  mat4 worldToLocal;
  mat4 fproj;
  mat4 bproj;
  mat4 lproj;
  mat4 rproj;
  mat4 uproj;
  mat4 dproj;
};

layout(binding = 2) uniform PointLightBufferObject{
    PointLightBufferItem lights[MAXLIGHTS];
    int totalLights;
} plbo;

layout(binding = 3) uniform MaterialBufferObject {
  vec4 ka, kd, ks, kr;
  bool useRoughnessLod; float roughness;
  bool useDiffuseTexture, useSpecularTexture, useCubemapTexture, useGI, useShadowMap;
} mbo;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
  w_position = vec3(tbo.localToWorld * vec4(inPosition.xyz, 1.0));
  vec3 w_cameraPos = vec3(pbo.at[gl_ViewIndex].viewinv[3]);
  vec3 w_viewDir = w_position - w_cameraPos;
  
  w_normal = normalize(vec3(tbo.localToWorld * vec4(inNormal, 0)));
  vec3 reflection = reflect(w_viewDir, normalize(inNormal));
  /* From z up to y up */
  w_reflection.x = reflection.x;
  w_reflection.y = reflection.z;
  w_reflection.z = reflection.y;

  gl_Position = pbo.at[gl_ViewIndex].proj * pbo.at[gl_ViewIndex].view * vec4(w_position, 1.0);
  fragTexCoord = inTexCoord;
}