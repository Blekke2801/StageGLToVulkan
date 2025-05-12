#version 450

//serve flat per ottenere il flat shading
layout(location = 0) in flat vec3 fragColor;

layout(location = 0) out vec4 outColor;

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

vec3 I_amb;

void main() {
    I_amb =  fragColor * (ubo.ambientLight.color * ubo.ambientLight.intensity);
    outColor = vec4(I_amb, 1.0);
}