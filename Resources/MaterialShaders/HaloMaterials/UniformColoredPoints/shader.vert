#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
    mat4 viewinv;
    mat4 projinv;
} cbo;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    vec4 color;
    float pointSize;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main() {
    gl_Position = cbo.proj * cbo.view * ubo.model * vec4(inPosition, 1.0);
    gl_PointSize = ubo.pointSize;
    fragColor = ubo.color;
}