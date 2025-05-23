#version 450

//input della shader
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec2 fragTextCoord;
layout(location = 3) flat in uint textureIndex;

//output della shader
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

layout(push_constant) uniform PushConstants {
    uint textureIndex;
} push;

layout(binding = 1) uniform sampler2D textures[8];

void main() {
	vec4 material_color = texture(textures[push.textureIndex], fragTextCoord);

	vec3 normal = normalize(fragNormal);
	vec3 lightDir = normalize(ubo.pointLight.position - fragPos); 
	float cosTheta = max(dot(normal, lightDir), 0.0);

	vec3 view_dir    = normalize(ubo.cameraPos.xyz - fragPos);
	vec3 reflect_dir = normalize(reflect(lightDir, normal));
	float cosAlpha = max(dot(view_dir, reflect_dir), 0.0);

	vec3 I_spec = material_color.rgb * (ubo.pointLight.color * ubo.specularLight.intensity) * pow(cosAlpha,ubo.specularLight.shininess);
    vec3 I_amb =  material_color.rgb * (ubo.ambientLight.color * ubo.ambientLight.intensity);
	vec3 I_dif = material_color.rgb * (ubo.pointLight.color * ubo.diffusiveLight.intensity) * cosTheta;


	outColor = vec4(I_amb + I_dif + I_spec, material_color.a); 
}