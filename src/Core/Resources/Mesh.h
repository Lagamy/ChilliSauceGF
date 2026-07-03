#pragma once 
#include "Pool.h"
#include "Vertex.h"
#include "UploadId.h"
#include <cstdint>
#include <vector> 
#include <glm/glm.hpp>

namespace Graphics
{
	struct Mesh { 
		UploadId vbMemoryId; // Handle to mem entry in the GPUMemoryManager
		UploadId ibMemoryId;

		std::vector<Vertex> vertices; 
		std::vector<uint32_t> indices; 
	};
} 
