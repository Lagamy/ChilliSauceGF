#pragma once 
#include "PoolMap.h"
#include "Utilities.h"
#include "Vertex.h"
#include "UploadId.h"
#include "DataContainer.h"
#include "Layout.h"
#include <cstdint>
#include <vector> 
#include <glm/glm.hpp>

namespace Graphics
{
	ReflectionLayout& getVerticeLayout(PoolId layoutId); // Forward declaration of function. Implementation is in API

	struct Mesh { 
		std::string name; 
		PoolId verticeLayoutId; 
		PoolId vertexDataContainerId; 
		std::vector<uint32_t> indices;

		UploadId vbMemoryUploadId; // Handle to mem entry in the GPUMemoryManager
		UploadId ibMemoryUploadId;

		template<typename T>
		void setVerticeMember(PoolId memberId_, uint32_t repeateUnitId_, T data_)
		{
			getVerticeLayout(this->verticeLayoutId).setMemberInDataContainer(memberId_, data_, this->vertexDataContainerId, repeateUnitId_);
		}
		void setVerticeDataContainer(void* data_, size_t offset_, size_t size_);
		void* getData();
		size_t getSize();
		void queueStaticGPUUpload();

		Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_); 
	};
} 
