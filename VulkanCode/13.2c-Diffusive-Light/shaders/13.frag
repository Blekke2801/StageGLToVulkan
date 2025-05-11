#version 450

//serve flat per ottenere il flat shading
layout(location = 0) in flat vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 transform;
    mat4 view;
    mat4 proj;
    vec3 ambientColor; //colore della luce ambientale
    float ambientLightIntensity; //intensità della luce ambientale
} ubo;

vec3 I_amb;

void main() {
    I_amb =  fragColor * (ubo.ambientColor * ubo.ambientLightIntensity);
    outColor = vec4(I_amb, 1.0);
}