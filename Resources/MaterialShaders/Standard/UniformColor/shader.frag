#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 2) uniform MaterialBufferObject {
    vec4 color;
    float pointSize;
    bool useTexture;
} mbo;

layout(binding = 3) uniform sampler2D colorTex;

void main() {
	if (mbo.useTexture) {
		outColor = texture(colorTex, fragTexCoord);
	}
	else {
    	outColor = fragColor;
	}
}