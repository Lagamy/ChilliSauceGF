#pragma once 
#include "Vertex.h"
#include <cstdint>
#include <vector> 
#include <glm/glm.hpp>

struct Mesh { 
	uint32_t vbMemoryId; // Handle to mem entry in the GPUMemoryManager
	uint32_t ibMemoryId;

	std::vector<Vertex> vertices; 
	std::vector<uint32_t> indices; 
}; 
