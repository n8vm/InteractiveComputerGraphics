#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAXLIGHTS 3

layout(binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} cbo;

layout(binding = 1) uniform UniformBufferObject {
  mat4 modelMatrix;
  mat4 normalMatrix;
  vec4 ka;
  vec4 kd;
  vec4 ks;
  bool useSpecular;
} ubo;

struct LightObject {
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

layout(binding = 2) uniform LightBufferObject {
 LightObject lights[MAXLIGHTS];
 int numLights;
} lbo;

layout(binding = 3) uniform sampler2D diffuseSampler;
layout(binding = 4) uniform sampler2D specSampler;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 position;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	/*Blinn-Phong shading model
		I = ka + Il * kd * (l dot n) + Il * ks * (h dot n)^N */	

	vec4 diffuseTexColor = texture(diffuseSampler, fragTexCoord);
	vec4 specTexColor = texture(specSampler, fragTexCoord);

	vec3 ambientColor = vec3(0.0,0.0,0.0);
	vec3 diffuseColor = vec3(0.0,0.0,0.0);
	vec3 specularColor = vec3(0.0,0.0,0.0);

	/* All computations are in eye space */
	vec3 v = vec3(0.0, 0.0, 1.0);
	vec3 n = normalize(normal);
	for (int i = 0; i < lbo.numLights; ++i) {
		vec3 l = normalize(vec3(cbo.view * lbo.lights[i].position) - position);
		vec3 h = normalize(v + l);
		float diffterm = max(dot(l, n), 0.0);
		float specterm = max(dot(h, n), 0.0);

		ambientColor += diffuseTexColor.rgb * ubo.ka.rgb * lbo.lights[i].ambient.rgb;

		diffuseColor += diffuseTexColor.rgb * ubo.kd.rgb * lbo.lights[i].diffuse.rgb * diffterm;
		
		specularColor += specTexColor.rgb * ubo.ks.rgb * lbo.lights[i].specular.rgb * pow(specterm, 80.);
	}

    outColor = vec4(diffuseColor, ubo.kd.a * diffuseTexColor.a) + vec4(specularColor, ubo.ks.a * specTexColor.a) + vec4(ambientColor, ubo.ka.a * diffuseTexColor.a);
}