#version 450 // Use GLSL 4.5

// from cpu side 
layout(location = 0) in vec3 pos; 
layout(location = 1) in vec4 col;

layout(set = 0, binding = 0) uniform  VP 
{
	mat4 projection;
	mat4 view; 
} vp; 

struct Object { vec4 color; int rotationDir; };

layout(set = 0, binding = 1, std430) readonly buffer Objects
{
    Object states[2];          // indexed in the shader
    mat4 model;                // one per triangle, not part of the states
} objects;                     // NOT an array of descriptors: the bind already picked the triangle

// layout(set = 0, binding = 2) uniform Counter { uint value; } counter;

layout(push_constant) uniform Counter { uint value; } counter; 

// to fragment shader
layout(location = 0) out vec4 fragCol; 

mat4 rotateBy(mat4 m, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return m * mat4(
        c,   -s,    0.0, 0.0,
        s,    c,    0.0, 0.0,
        0.0,  0.0,  1.0, 0.0,
        0.0,  0.0,  0.0, 1.0
    );
}

void main() 
{
	uint id = 0; 
	if(counter.value > 7000)
	{	
		id = 1; 
	}
	fragCol = col * objects.states[id].color;
	mat4 model = rotateBy(objects.model, float(counter.value) * float(objects.states[id].rotationDir) / 500.0f);
	gl_Position = vp.projection * vp.view * model * vec4(pos, 1.0);
}
