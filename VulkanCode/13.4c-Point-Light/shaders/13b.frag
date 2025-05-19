#version 450

//serve flat per ottenere il flat shading
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

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
// diffusiva
struct DiffusiveLightStruct {
	float intensity;
};

struct SpecularLightStruct {
	float intensity;
	float shininess;
};

// Struttura dati di lavoro per contenere le informazioni sulla luce
// puntiforme
struct PointLightStruct {
	vec3 color;
	vec3 position;
};

layout(binding = 0) uniform UniformBufferObject{
    SceneMatrices scene;
    AmbientLight ambientLight;
    PointLightStruct pointLight;// sostituisce pointLight
    DiffusiveLightStruct diffusiveLight;
	SpecularLightStruct specularLight;
    vec4 cameraPos;
} ubo;



void main() {
	// Normalizziamo il vettore delle normali
	vec3 normal = normalize(fragNormal);
	vec3 lightDir = normalize(ubo.pointLight.position - fragPos); 
	float cosTheta = max(dot(normal, lightDir), 0.0);

	vec3 view_dir    = normalize(ubo.cameraPos.xyz - fragPos);
	vec3 reflect_dir = normalize(reflect(lightDir, normal));
	float cosAlpha = max(dot(view_dir, reflect_dir), 0.0);

	vec3 I_spec = fragColor * (ubo.pointLight.color * ubo.specularLight.intensity) * pow(cosAlpha,ubo.specularLight.shininess);
    vec3 I_amb =  fragColor * (ubo.ambientLight.color * ubo.ambientLight.intensity);
	vec3 I_dif = fragColor * (ubo.pointLight.color * ubo.diffusiveLight.intensity) * cosTheta;


	outColor = vec4(I_amb + I_dif + I_spec, 1.0); 
}