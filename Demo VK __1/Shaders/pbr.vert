#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObjectVP
{
    mat4 view;
    mat4 proj;
} uboVP;

// Per vertex
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

// Per instance
layout(location = 4) in mat4 model;
layout(location = 8) in vec3 inAlbedo;
layout(location = 9) in vec3 inRoughness;
layout(location = 10) in vec3 inMetal;

// Out
layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 normal;

layout(location = 5) out vec3 outAlbedo;
layout(location = 6) out vec3 outRoughness;
layout(location = 7) out vec3 outMetal;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = uboVP.proj * uboVP.view * model * vec4(inPosition, 1.0);
	
	mat3 usedModelMatrix = transpose(inverse(mat3(model)));
    normal = usedModelMatrix * inNormal;
	/*vec3 t = normalize(usedModelMatrix * inTangent);
	//t = normalize(t - dot(t, n) * n);
	vec3 b = normalize(cross(t, n));
	tbn = inverse(mat3(t, b, n));*/
	
	worldPos = vec3(model * vec4(inPosition, 1.0));
    fragTexCoord = inTexCoord;

    outAlbedo = inAlbedo;
    outRoughness = inRoughness;
    outMetal = inMetal;
}