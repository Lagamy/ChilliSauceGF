#pragma once 
#include "Pool.h"
#include "Utilities.h"
#include "Vertex.h"
#include "UploadId.h"
#include "DataContainer.h"
#include <cstdint>
#include <vector> 
#include <glm/glm.hpp>

namespace Graphics
{
	struct Mesh { 
		UploadId vbMemoryId; // Handle to mem entry in the GPUMemoryManager
		UploadId ibMemoryId;
		bool created = false; 

		PoolId verticeLayoutId; 
		PoolId vertexDataContainerId; 
		std::vector<uint32_t> indices;

		void create(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_);
		template<typename T>
		void setVerticeMember(PoolId memberId_, uint32_t repeateUnitId_, T data_);
		void setVerticeDataContainer(void* data_, size_t offset_, size_t size_);
		
		void* getData();
		size_t getSize();
	};
} 
