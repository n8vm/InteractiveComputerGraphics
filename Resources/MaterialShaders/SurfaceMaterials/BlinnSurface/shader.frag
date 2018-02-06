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
} ubo;

struct LightObject { /* TODO: maybe change this name */
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	//float pad, constantAttenuation, linearAttenuation, quadraticAttenuation;
	// float constantAttenuation, linearAttenuation, quadraticAttenuation;
	// float spotCutoff, spotExponent;
	// vec3 spotDirection;
};

layout(binding = 2) uniform LightBufferObject {
 LightObject lights[MAXLIGHTS];
 int numLights;
} lbo;


layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 position;

layout(location = 0) out vec4 outColor;

void main() {
	/*Blinn-Phong shading model
		I = ka + Il * kd * (l dot n) + Il * ks * (h dot n)^N */	

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

		ambientColor += vec3(ubo.ka) * vec3(lbo.lights[i].ambient);

		diffuseColor += vec3(ubo.kd) * vec3(lbo.lights[i].diffuse) * diffterm;
		
		specularColor += vec3(lbo.lights[i].specular) * vec3(ubo.ks) * pow(specterm, 80.);
	}

    outColor = vec4(diffuseColor + specularColor + ambientColor, 1.0);
}