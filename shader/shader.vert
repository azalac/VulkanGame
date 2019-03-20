#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec3 vertColor;
layout(location = 2) in vec2 vertUV;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform ObjectInfo {
    vec2 position;
    vec2 size;
    float rotation;
    vec2 aspect;
} info;

//https://gist.github.com/yiwenl/3f804e80d0930e34a0b33359259b556c
mat2 rotation(float a) {
    float s = sin(a);
    float c = cos(a);
    mat2 m = mat2(c, s, -s, c);
    return m;
}

void main() {
    vec2 pos = rotation(info.rotation) * vertPos * info.size + info.position;

    gl_Position = vec4(pos, 0.0, 1.0);
    fragColor = vec3(vertUV, 0);
}
