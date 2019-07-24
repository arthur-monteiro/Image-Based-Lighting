#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
	if(fragTexCoord.x < 0.03 || fragTexCoord.x > 0.98 || fragTexCoord.y < 0.014 || fragTexCoord.y > 0.98) // elimine les résidus
		discard;
    outColor = vec4(texture(texSampler, fragTexCoord).rrrr);
}