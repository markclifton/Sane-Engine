#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in float aTexID;

layout (location = 0) out vec3 FragPos;
layout (location = 1) out vec2 TexCoords;
layout (location = 2) out vec3 Normal;
layout (location = 3) out float TexID;

layout(binding = 0) uniform UniformBufferObject {
    mat4 projection;
    mat4 view;
};

void main()
{
    vec4 worldPos = vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    TexCoords = aTexCoords;

    mat3 model = transpose(inverse(mat3(1.0)));
    Normal = model * aNormal;

    gl_Position = projection * view * worldPos;

    TexID = aTexID;
}
