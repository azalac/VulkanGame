#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D sprite;

void main() {
    outColor = texture(sprite, vec2(1 - fragColor.x, fragColor.y));
}
