#version 450

//serve flat per ottenere il flat shading
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;

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

layout(binding = 0) uniform UniformBufferObject{
    SceneMatrices scene;
    AmbientLight ambientLight;
    DirectionalLightStruct directionalLight;
    DiffusiveLightStruct diffusiveLight;
} ubo;



void main() {
    vec3 I_amb =  fragColor * (ubo.ambientLight.color * ubo.ambientLight.intensity);

	vec3 I_dif = vec3(0,0,0);

	// Normalizziamo il vettore delle normali
	vec3 normal = normalize(fragNormal);

	// La direzione della luce è un vettore che parte dalla luce fino 
	// agli oggetti. E' necessario invertire la direzione prima di 
	// calcolare il fattore coseno.
	// Bisogna sempre essere sicuri di avere le normali in forma di 
	// versori  
	float cosTheta = max(dot(normal, ubo.directionalLight.direction), 0.0);
	I_dif = fragColor * (ubo.directionalLight.color * ubo.diffusiveLight.intensity) * cosTheta;


	outColor = vec4(vec3(cosTheta), 1.0);
}