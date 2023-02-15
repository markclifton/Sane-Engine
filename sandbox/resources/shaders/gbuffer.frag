#version 450

layout (location = 0) in vec3 FragPos;
layout (location = 1) in vec2 TexCoords;
layout (location = 2) in vec3 Normal;
layout (location = 3) in float TexID;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

#extension GL_EXT_nonuniform_qualifier : enable
layout (binding = 1) uniform sampler2D textures[];

void main()
{
    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = texture(textures[int(TexID + .5f)], TexCoords).xyz;
    gAlbedoSpec.a = .5f; 
} 