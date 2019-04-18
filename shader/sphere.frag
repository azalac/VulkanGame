#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform ChromaKey {
    vec4 search_col;
    vec4 replace_col;
    float epsilon1, epsilon2;
} key;

layout(binding = 1) uniform sampler2D sprite;

void main() {
    vec2 uv = vec2(1 - fragColor.x, fragColor.y);

    vec4 tex = texture(sprite, uv);

    float d = distance(tex, key.search_col);

    float a = max(d - key.epsilon1, 0) / (key.epsilon2 - key.epsilon1);

    a = min(a, 1);

    vec4 col = tex - key.search_col * (1-a);

    // the 0 .. 1 direction-ness of the sun
    // power of 3 to increase constrast at equator but keep the sign
    float _dot = uv.x + uv.y;

    outColor = mix(col, vec4(0, 0, 0, col.w), pow(uv.x + uv.y, 4));
}
