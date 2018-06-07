#version 450
#extension GL_ARB_separate_shader_objects : enable
precision highp float;

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

layout(binding = 4) uniform sampler2D diffuseSampler;
layout(binding = 5) uniform sampler2D specSampler;
layout(binding = 6) uniform samplerCube samplerCubeMap;
layout(binding = 7) uniform sampler3D volumeTexture;
layout(binding = 8) uniform samplerCube shadowMap; /* Todo: make this a cubemap array. */

layout(location = 0) in vec3 w_normal;
layout(location = 1) in vec3 w_position;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 w_reflection;

layout(location = 0) out vec4 outColor;


float PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
float PI  = 3.14159265358979323846264 * 00000.1; // PI
float SQ2 = 1.41421356237309504880169 * 10000.0; // Square Root of Two

float gold_noise(in vec2 coordinate, in float seed){
    return fract(sin(dot(coordinate*(seed+PHI), vec2(PHI, PI)))*SQ2);
}

#define VOXEL_SIZE (1/128.0)
#define MIPMAP_HARDCAP 20.f
#define MIPMAP_OFFSET .5f
#define TSQRT2 2.828427
#define SQRT2 1.414213
//#define SQRT2 2.0
#define ISQRT2 0.707106
#define DIFFUSE_INDIRECT_FACTOR 1.f /* Just changes intensity of diffuse indirect lighting. */

#define diffuseStepSize (VOXEL_SIZE)

vec3 scaleAndBias(const vec3 p) { return 0.5f * p + vec3(0.5f); }

// Returns a vector that is orthogonal to u.
vec3 orthogonal(vec3 u){
  u = normalize(u);
  vec3 v = vec3(0.99146, 0.11664, 0.05832); // Pick any normalized vector.
  return abs(dot(u, v)) > 0.99999f ? cross(u, vec3(0, 1, 0)) : cross(u, v);
}

// Returns true if the point p is inside the unit cube. 
bool isInsideCube(const vec3 p, float e) { return abs(p.x) < 1 + e && abs(p.y) < 1 + e && abs(p.z) < 1 + e; }


// Traces a diffuse voxel cone.
vec3 traceDiffuseVoxelCone(vec3 from, vec3 direction){
  direction = normalize(direction);
  from *= .1;
  float coneSpread = 0.325;

  vec4 acc = vec4(0.0f);

  // Controls bleeding from close surfaces.
  // Low values look rather bad if using shadow cone tracing.
  // Might be a better choice to use shadow maps and lower this value.
  float dist = 10.0 * VOXEL_SIZE;

  // Trace.
  while(dist < SQRT2 && acc.a < 1){
    vec3 c = from + dist * direction;
    c = scaleAndBias(from + dist * direction);
    float l = (1 + coneSpread * dist / VOXEL_SIZE);
    float level = log2(l);
    float ll = (level + 1) * (level + 1);
    vec4 voxel = textureLod(volumeTexture, c, min(MIPMAP_HARDCAP, level + MIPMAP_OFFSET));
    acc += .1*ll * voxel * pow(1 - voxel.a, 2);
    dist += ll * diffuseStepSize;
  }
  return acc.rgb;//pow(acc.rgb * 2.0, vec3(1.5));
}

// Returns a soft shadow blend by using shadow cone tracing.
// Uses 2 samples per step, so it's pretty expensive.
float traceShadowCone(vec3 from, vec3 direction, float targetDistance){
  from += w_normal * 0.05f; // Removes artifacts but makes self shadowing for dense meshes meh.

  float acc = 0;

  float dist = 3 * VOXEL_SIZE;
  // I'm using a pretty big margin here since I use an emissive light ball with a pretty big radius in my demo scenes.
  const float STOP = targetDistance - 32 * VOXEL_SIZE;

  while(dist < STOP && acc < 1){  
    vec3 c = from + dist * direction;
    if(!isInsideCube(c, 0)) break;
    c = scaleAndBias(c);
    float l = pow(dist, 2); // Experimenting with inverse square falloff for shadows.
    float s1 = 0.32 * textureLod(volumeTexture, c, 0.75 * l).a;
    float s2 = 0.15 * textureLod(volumeTexture, c, 4.5 * l).a;
    float s = s1 + s2;
    acc += (1 - acc) * s;
    dist += 0.9 * VOXEL_SIZE * (1 + 0.05 * l);
  }
  return 1 - pow(smoothstep(0, 1, acc * 1.4), 1.0 / 1.4);
}

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);   

float traceShadowMap(int lightIndex, vec3 w_from, vec3 w_light) {
  // get vector between fragment position and light position
  vec3 fragToLight = w_from - w_light;
  // now get current linear depth as the length between the fragment and light position
  float currentDepth = length(fragToLight);

  float farPlane = 1000.f; // todo: read from pbo

  float shadow = 0.0;
  float bias   = 0.2;
  int samples  = 20;
  float viewDistance = currentDepth;//length(viewPos - fragPos);
  float diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0;  
  for(int i = 0; i < samples; ++i)
  {
      float closestDepth = texture(shadowMap, fragToLight.xzy + sampleOffsetDirections[i] * diskRadius).r;
      closestDepth *= farPlane;   // Undo mapping [0;1]

      shadow += 1.0f;
      if((currentDepth - closestDepth) > 0.15)
          shadow -= min(1.0, pow((currentDepth - closestDepth), .1));
  }
  shadow /= float(samples);  

  return shadow;
}


vec3 getDirectIllumination() {
  /*Blinn-Phong shading model
    I = ka + Il * kd * (l dot n) + Il * ks * (h dot n)^N */ 

  vec4 diffMatColor = (mbo.useDiffuseTexture) ? texture(diffuseSampler, fragTexCoord) : mbo.kd;
  vec4 specMatColor = (mbo.useSpecularTexture) ? texture(specSampler, fragTexCoord) : mbo.ks;

  vec3 ambientColor = vec3(0.0,0.0,0.0);
  vec3 diffuseColor = vec3(0.0,0.0,0.0);
  vec3 specularColor = vec3(0.0,0.0,0.0);
  ambientColor += vec3(mbo.ka);


  /* All computations are in eye space */
  vec3 v = vec3(0.0, 0.0, 1.0);
  for (int i = 0; i < plbo.totalLights; ++i) {
    vec3 lightPos = vec3(plbo.lights[i].localToWorld[3]);
    vec3 lightCol = vec3(plbo.lights[i].color) * plbo.lights[i].intensity;
    float lightDist = distance(lightPos , w_position);
    vec3 l = normalize(lightPos - w_position);
    vec3 h = normalize(v + l);
    float diffterm = max(dot(l, w_normal), 0.0);
    float specterm = max(dot(h, w_normal), 0.0);

    float shadowBlend = 1;
    if(mbo.useShadowMap && diffterm > 0)
      shadowBlend = traceShadowMap(i, w_position, lightPos);
      //shadowBlend = traceShadowCone(w_position, l, lightDist);
    float dist = 1.0;
    if (plbo.lights[i].falloffType == 1) dist /= (1.0 + pow(lightDist, 2.0f));
    else if (plbo.lights[i].falloffType == 2) dist /= (1.0 + lightDist);

    diffuseColor += vec3(diffMatColor) * shadowBlend * lightCol * diffterm * dist;
    
    specularColor += vec3(specMatColor) * shadowBlend * lightCol * pow(specterm, 80.) * dist;
  }

  return diffuseColor + ambientColor + specularColor;
}

vec3 getIndirectIllumination() {
  vec3 acc = vec3(0.0);
  if(mbo.useGI) {
    float ANGLE_MIX = .5; // Angle mix (1.0f => orthogonal direction, 0.0f => direction of normal).

    /* Get a base derived from the normal */
    vec3 ortho = normalize(orthogonal(w_normal));
    vec3 ortho2 = normalize(cross(ortho, w_normal));

    float random = gold_noise(gl_FragCoord.xy, 0.0);

    // Find base vectors for the corner cones too.
    vec3 corner = normalize(ortho + ortho2);
    vec3 corner2 = normalize(ortho - ortho2);

    // Find start position of trace (start with a bit of offset).
    vec3 normalOffset = w_normal * (1 + 4 * ISQRT2) * VOXEL_SIZE;
    vec3 pos = w_position + normalOffset;

    // We offset forward in normal direction, and backward in cone direction.
    // Backward in cone direction improves GI, and forward direction removes
    // artifacts.
    float CONE_OFFSET = 0.0;//-0.1;

    // Trace front cone
    acc += traceDiffuseVoxelCone(pos + CONE_OFFSET * w_normal, w_normal);

    // Trace 4 side cones.
    vec3 s1 = mix(w_normal, ortho, ANGLE_MIX);
    vec3 s2 = mix(w_normal, -ortho, ANGLE_MIX);
    vec3 s3 = mix(w_normal, ortho2, ANGLE_MIX);
    vec3 s4 = mix(w_normal, -ortho2, ANGLE_MIX);

    acc += traceDiffuseVoxelCone(pos + CONE_OFFSET * ortho, s1) * dot(s1, w_normal);
    acc += traceDiffuseVoxelCone(pos - CONE_OFFSET * ortho, s2) * dot(s2, w_normal);
    acc += traceDiffuseVoxelCone(pos + CONE_OFFSET * ortho2, s3) * dot(s3, w_normal);
    acc += traceDiffuseVoxelCone(pos - CONE_OFFSET * ortho2, s4) * dot(s4, w_normal);

    // Trace 4 corner cones.
    vec3 c1 = mix(w_normal, corner, ANGLE_MIX);
    vec3 c2 = mix(w_normal, -corner, ANGLE_MIX);
    vec3 c3 = mix(w_normal, corner2, ANGLE_MIX);
    vec3 c4 = mix(w_normal, -corner2, ANGLE_MIX);

    acc += traceDiffuseVoxelCone(pos + CONE_OFFSET * corner, c1) * dot(c1, w_normal);
    acc += traceDiffuseVoxelCone(pos - CONE_OFFSET * corner, c2) * dot(c2, w_normal);
    acc += traceDiffuseVoxelCone(pos + CONE_OFFSET * corner2, c3) * dot(c3, w_normal);
    acc += traceDiffuseVoxelCone(pos - CONE_OFFSET * corner2, c4) * dot(c4, w_normal);
  }

  return acc;
}

vec3 getReflectedIllumination() {
  vec3 reflectedIllumination = vec3(0.0, 0.0, 0.0);

  if(mbo.useCubemapTexture) {
    if (mbo.useRoughnessLod) {
      reflectedIllumination += vec3(textureLod(samplerCubeMap, w_reflection, mbo.roughness) * mbo.kr);
    } else {
      reflectedIllumination += vec3(texture(samplerCubeMap, w_reflection) * mbo.kr);
    }
  }

  return reflectedIllumination;
}

void main() {
  vec4 diffMatColor = (mbo.useDiffuseTexture) ? texture(diffuseSampler, fragTexCoord) : mbo.kd;


  vec3 directIllumination = getDirectIllumination();
  vec3 indirectIllumination = (mbo.useGI) ? getIndirectIllumination() * diffMatColor.rgb : vec3(0.0);
	vec3 reflectedIllumination = (mbo.useCubemapTexture) ? getReflectedIllumination() : vec3(0.0);

  outColor = vec4(directIllumination + indirectIllumination * DIFFUSE_INDIRECT_FACTOR + reflectedIllumination, 1.0);  

  /* Gamma correction */
  outColor.rgb = pow(outColor.rgb, vec3(1.0 / 2.2));
}