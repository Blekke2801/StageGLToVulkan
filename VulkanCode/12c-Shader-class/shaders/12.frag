#version 450

//serve flat per ottenere il flat shading
layout(location = 0) in flat vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}