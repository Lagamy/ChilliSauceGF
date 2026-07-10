#pragma once
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

struct Vertex { 
	glm::vec3 position;
	// glm::vec3 normal; 
	// glm::vec2 UV;  

	Vertex(glm::vec3 position_) : position(position_) {};
};
