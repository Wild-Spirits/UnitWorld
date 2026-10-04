#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

vec3 colors[3] = vec3[](vec3(0.8, 0.2, 0.1), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

layout(set = 0, binding = 0) uniform perFrameUbo
{
    mat4 view;
    mat4 proj;
}
frameUbo;

layout(set = 1, binding = 0) uniform texture2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler albedoSampler;

// Only guaranteed a total of 128 bytes.
layout(push_constant) uniform perDrawUbo
{
    mat4 model;    // 64 bytes
}
drawUbo;

layout(location = 0) out struct dto
{
    vec2 texCoord;
} outDto;

void main()
{
    outDto.texCoord = vec2(inTexCoord.x, 1.0 - inTexCoord.y);
    gl_Position = frameUbo.proj * frameUbo.view * drawUbo.model * vec4(inPosition, 1.0);
}
