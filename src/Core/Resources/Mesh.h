#pragma once 
#include "Pool.h"
#include "Vertex.h"
#include <cstdint>
#include <vector> 
#include <glm/glm.hpp>

struct Mesh { 
	PoolId vbMemoryId; // Handle to mem entry in the GPUMemoryManager
	PoolId ibMemoryId;

	std::vector<Vertex> vertices; 
	std::vector<uint32_t> indices; 
}; 
