#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

//viene usato flat in modo da non far mescolare i colori
layout(location = 0) flat out vec3 fragColor;

struct SceneMatrices {
    mat4 transform;
    mat4 view;
    mat4 proj;
};

struct AmbientLight {
    vec3 color;
    float intensity;
};

layout(binding = 0) uniform UniformBufferObject{
    SceneMatrices scene;
    AmbientLight ambientLight;
} ubo;

void main()
{
    gl_Position = ubo.scene.proj * ubo.scene.view * ubo.scene.transform * vec4(inPosition, 1.0);
    fragColor = inColor;
}