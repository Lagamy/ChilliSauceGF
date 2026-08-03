#version 450 // Use GLSL 4.5

// from cpu side 
layout(location = 0) in vec3 pos; 
layout(location = 1) in vec4 col;

// layout(binding = 0, set = 0) uniform MVP {
	// mat4 projection; 
	// mat4 view; 
	// mat4 model; 
// } mvp; 

// to fragment shader
layout(location = 0) out vec4 fragCol; 

void main() {
	// gl_Position = mvp.projection * mvp.view * mvp.model * vec4(pos, 1.0);
	fragCol = col;
	gl_Position = vec4(pos, 1.0); 
}
