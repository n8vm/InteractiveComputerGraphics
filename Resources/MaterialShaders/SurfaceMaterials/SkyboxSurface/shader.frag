#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
    mat4 viewinv;
    mat4 projinv;
} cbo;

layout (binding = 2) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;
layout (location = 1) in vec3 camPos;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(samplerCubeMap, inUVW - camPos);
}