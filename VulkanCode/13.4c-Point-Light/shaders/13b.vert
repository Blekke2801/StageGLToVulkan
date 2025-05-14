#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 normal;

//viene usato flat in modo da non far mescolare i colori
layout(location = 0) out vec3 fragColor;

layout(location = 1) out vec3 fragNormal;

layout(location = 2) out vec3 fragPosition;

struct SceneMatrices {
    mat4 transform;
    mat4 view;
    mat4 proj;
};

struct AmbientLight {
    vec3 color;
    float intensity;
};

// Struttura dati di lavoro per contenere le informazioni sulla luce
// direzionale
struct DirectionalLightStruct {
	vec3 color;
	vec3 direction;
};

// Struttura dati di lavoro per contenere le informazioni sulla luce
// diffusiva
struct DiffusiveLightStruct {
	float intensity;
};

struct SpecularLightStruct {
	float intensity;
	float shininess;
};

layout(binding = 0) uniform UniformBufferObject{
    SceneMatrices scene;
    AmbientLight ambientLight;
    DirectionalLightStruct directionalLight;
    DiffusiveLightStruct diffusiveLight;
	SpecularLightStruct specularLight;
    vec4 cameraPos;
} ubo;

void main()
{
    gl_Position = ubo.scene.proj * ubo.scene.view * ubo.scene.transform * vec4(inPosition, 1.0);
    fragNormal = (transpose(inverse(ubo.scene.transform)) * vec4(normal,0.0)).xyz;
    fragColor = inColor;
    fragPosition = (ubo.scene.transform * vec4(inPosition,1.0)).xyz;
}