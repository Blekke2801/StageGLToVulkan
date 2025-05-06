#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

//viene usato flat in modo da non far mescolare i colori
layout(location = 0) flat out vec3 fragColor;

//essendo oggetto 3d, oltre alla trasformazione, dobbiamo inserire anche la vista e la proiezione sulla cam
layout(binding = 0) uniform UniformBufferObject {
    mat4 transform;
    mat4 view;
    mat4 proj;
} ubo;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.transform * vec4(inPosition, 1.0);
    fragColor = inColor;
}