#version 450

// Struttura dati di lavoro per contenere le informazioni sulla luce
// ambientale
struct AmbientLightStruct {
	vec3 color;
	float intensity;
};

//serve flat per ottenere il flat shading
layout(location = 0) in flat vec3 fragColor;

layout(location = 0) out vec4 outColor;

// Informazioni di luce ambientale 
uniform AmbientLightStruct AmbientLight;

// Componente ambientale del colore dell'oggetto 
vec3 I_amb;

void main() {
    I_amb =  fragment_color * (AmbientLight.color * AmbientLight.intensity);
    outColor = vec4(I_amb, 1.0);
}