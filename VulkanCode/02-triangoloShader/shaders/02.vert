#version 330

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main()
{
	// Negli shader si usano le coordinate omogenee
	// In output mandiamo un punto in coordinate omogenee che ha
	// gl_Position è una variabile predefinita
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}