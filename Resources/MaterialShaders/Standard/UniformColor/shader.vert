#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_multiview : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

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
    PerspectiveObject perspectives[MAX_MULTIVIEW];
} pbo;

layout(binding = 1) uniform TransformBufferObject{
    mat4 worldToLocal;
    mat4 localToWorld;
} tbo;

layout(binding = 2) uniform MaterialBufferObject {
    vec4 color;
    float pointSize;
    bool useTexture;
} mbo;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main() {
    gl_Position = pbo.perspectives[gl_ViewIndex].proj * pbo.perspectives[gl_ViewIndex].view * tbo.localToWorld * vec4(inPosition, 1.0);
    gl_PointSize = mbo.pointSize;
    fragColor = mbo.color;
    fragTexCoord = inTexCoord;
}