#version 450

// from vertex shader
layout(location = 0) in vec4 fragCol; 
// to screen
layout(location = 0) out vec4 outColor; // output to attachment in location 0

void main() {
	outColor = fragCol; 
	// outColor = vec4(1.0, 0.0, 1.0, 1.0);
}
