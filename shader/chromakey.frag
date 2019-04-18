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

bool within(vec3 median, float range, vec3 x) {
    return all(greaterThanEqual(x, median - range)) && all(lessThanEqual(x, median + range));
}

//https://stackoverflow.com/a/17897228
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
	vec4 tex = texture(sprite, vec2(1 - fragColor.x, fragColor.y));
    
	float d = distance(tex, key.search_col);
	
	float a = max(d - key.epsilon1, 0) / (key.epsilon2 - key.epsilon1);
	
	a = min(a, 1);
	
	outColor = tex - key.search_col * (1-a);
	
}
