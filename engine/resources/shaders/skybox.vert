#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(push_constant) uniform Push {
  mat4 transform;
} push;

void main() {
    gl_Position = push.transform * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}