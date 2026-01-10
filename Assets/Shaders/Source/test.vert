#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;

// vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
vec3 colors[3] = vec3[](vec3(0.8, 0.2, 0.1), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

// Only guaranteed a total of 128 bytes.
layout(push_constant) uniform perDrawUbo
{
    mat4 model;    // 64 bytes
}
drawUbo;

void main()
{
    gl_Position = drawUbo.model * vec4(inPosition, 1.0);
    fragColor = colors[gl_VertexIndex];
}
