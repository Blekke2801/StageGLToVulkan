#version 330

// Questo fragment shader si limita a colorare di rosso i punti della scena

// L'output di un fragment shader è sempre un colore. 
// In questo cas è nel formato R,G,B,A e quindi è un vettore a 4 componenti.
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}