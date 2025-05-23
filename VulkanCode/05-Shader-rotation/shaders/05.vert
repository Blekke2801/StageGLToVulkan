#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UniformBufferObject {
    mat4 transform;
} ubo;

void main()
{
    gl_Position = ubo.transform * vec4(inPosition, 1.0);
}