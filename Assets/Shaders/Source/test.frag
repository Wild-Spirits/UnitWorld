#version 450

layout(set = 0, binding = 0) uniform texture2D albedoTexture;
layout(set = 0, binding = 1) uniform sampler albedoSampler;

layout(location = 0) in struct dto {
	vec2 texCoord;
} inDto;

layout(location = 0) out vec4 outColor;

void main() 
{ 
    // outColor = vec4(inDto.texCoord, 0.0, 1.0);
    outColor = texture(sampler2D(albedoTexture, albedoSampler), inDto.texCoord); 
}
