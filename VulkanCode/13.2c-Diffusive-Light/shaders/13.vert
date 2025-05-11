#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 normal;

//viene usato flat in modo da non far mescolare i colori
layout(location = 0) flat out vec3 fragColor;

layout(location = 0) out vec4 fragNormal;

layout(binding = 0) uniform UniformBufferObject {
    mat4 transform;
    mat4 view;
    mat4 proj;
    vec3 ambientColor; //colore della luce ambientale
    float ambientLightIntensity; //intensità della luce ambientale
} ubo;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.transform * vec4(inPosition, 1.0);
    fragNormal = (ubo.transform * vec4(normal, 0.0)).xyz;
    fragColor = inColor;
}