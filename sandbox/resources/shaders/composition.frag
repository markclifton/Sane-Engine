 #version 450

layout (location = 0) in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

struct Light
{
    vec4 Position;
    vec4 Color;
    float Linear;
    float Quadratic;
};

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;
layout(binding = 3) uniform samplerCube gShadowMap;
layout(binding = 4) uniform PositionData {
    vec3 viewPos;
};
layout(binding = 5) uniform LightData {
    Light light;
};

#define EPSILON 0.20
#define SHADOW_OPACITY 0.05

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
); 

float ShadowCalculation(vec3 fragPos, float attenuation)
{
    float totalLight = 0.f;
    for(int i = 0; i < 20; ++i)
    {
        vec3 lightVec = (fragPos + sampleOffsetDirections[i]) - light.Position.xyz;
        float sampledDist = texture(gShadowMap, lightVec).r;
        float dist = length(lightVec);
	    totalLight += ((dist <= sampledDist + EPSILON) ? 1.0 : SHADOW_OPACITY);
    }

    return totalLight / 20.f * attenuation;
}

void main() {
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.5; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos.xyz - FragPos);

    // diffuse
    vec3 lightDir = normalize(light.Position.xyz - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color.xyz;

    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = light.Color.xyz * spec * Specular;

    // attenuation
    float distance = length(light.Position.xyz - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;

    lighting += diffuse + specular;
    lighting *= ShadowCalculation(FragPos, attenuation) * attenuation;

    if(FragPos != vec3(0,0,0)){
        FragColor = vec4(lighting, 1.0);
    } else {
        FragColor = vec4(Diffuse,0.0);
    }
}