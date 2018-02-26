#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;

layout(binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
    mat4 viewinv;
    mat4 projinv;
} cbo;

layout(binding = 1) uniform UniformBufferObject {
	mat4 modelMatrix;
	mat4 inverseModelMatrix;
} ubo;

layout (location = 0) out vec3 outUVW;
layout (location = 1) out vec3 camPos;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW.x = inPos.x;
	outUVW.y = inPos.z;
	outUVW.z = inPos.y;
	vec3 cameraPos = vec3(ubo.inverseModelMatrix * cbo.viewinv[3]);
	camPos.x = cameraPos.x;
	camPos.y = cameraPos.z;
	camPos.z = cameraPos.y;
	
	gl_Position = cbo.proj * cbo.view * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
