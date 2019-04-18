#version 450
#extension GL_ARB_separate_shader_objects : enable

float radius = 0.3, threshold = 0.02, post = 0.005, stability = 20.0, deviation = 0.001;
vec3 color = vec3(0.25, 0.5, 0.75);
vec3 polynomial = vec3(97, 101, 107);

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform ChromaKey {
    vec4 search_col;
    vec4 replace_col;
    float epsilon1, epsilon2;
} key;

// 1 on edges, 0 in middle
float hex(vec2 p) {
	p.x *= 0.57735*2.0;
	p.y += mod(floor(p.x), 2.0)*0.5;
	p = abs((mod(p, 1.0) - 0.5));
	return abs(max(p.x*1.5 + p.y, p.y*2.0) - 1.0);
}

void main()
{
    vec2 uv = vec2(1 - fragColor.x, fragColor.y);

    vec2 disp = uv - vec2(0.5);
    
    vec3 sins = sin(polynomial + atan(disp.x, disp.y));
    
    float dist = length(disp) + deviation * (sins.x + sins.y + sins.z);
    
    float k = (dist > radius ? post : threshold) / abs(radius - dist);
    
    vec3 col = color * k;

    vec4 grid = vec4(0.0);
    
    if(dist < radius) {
	vec2 p = uv * 20.0; 
	float  r = (1.0 -0.7)*0.5;
        float k2 = smoothstep(r, 0.0, hex(p)) * dist;
    
    	grid = vec4(k2 * color, k2);
    }
    
    // Output to screen
    outColor = (dist < radius ? grid : vec4(0.0)) + vec4(color * k, k);
}
