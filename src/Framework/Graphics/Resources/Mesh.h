#pragma once
#include "PoolMap.h"
#include "Utilities.h"
#include "Vertex.h"
#include "MemoryEntryId.h"
#include "DataContainer.h"
#include "Layout.h"
#include <cstdint>
#include <vector> 
#include <unordered_map>
#include <glm/glm.hpp>

namespace Graphics
{
	ReflectionLayout& getVerticeLayout(PoolId layoutId); // Forward declaration of function. Implementation is in API

	/* A run of neighbouring vertices with changed members. The byte range spans only the changed members inside it. */
	struct DirtyInMesh
	{
		uint32_t firstVerticeId;
		uint32_t lastVerticeId;
		uint64_t inSrcOffset;
		uint64_t inEntryOffset;
		uint64_t size;
	};

	struct Mesh {
		std::string name;
		PoolId verticeLayoutId;
		PoolId vertexDataContainerId;
		std::vector<uint32_t> indices;
		std::vector<DirtyInMesh> dirtyInMesh;
		std::unordered_map<uint32_t, uint32_t> verticeToDirtyId; // Vertice id -> index in dirtyInMesh of the run containing it

		MemoryEntryId vbMemoryUploadId; // Handle to mem entry in the GPUMemoryManager
		MemoryEntryId ibMemoryUploadId;

		template<typename T>
		void setVerticeMember(PoolId memberId_, uint32_t verticeId_, T data_)
		{
			getVerticeLayout(this->verticeLayoutId).setMemberInDataContainer(memberId_, data_, this->vertexDataContainerId, verticeId_);
			
		}

		template<typename T>
		const T getVerticeMember(PoolId memberId_, uint32_t verticeId_)
		{
			return getVerticeLayout(this->verticeLayoutId).getMemberInDataContainer<T>(memberId_, this->vertexDataContainerId, verticeId_);
		}

		template<typename T>
		void updateVerticeMember(PoolId memberId_, uint32_t verticeId_, T data_)
		{
			this->setVerticeMember(memberId_, verticeId_, data_);
			this->markVerticeDirty(memberId_, verticeId_);
		}

		void setVerticeDataContainer(void* data_, size_t offset_, size_t size_);
		void* getData();
		size_t getSize();
		void queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_);
		void markVerticeDirty(PoolId memberId_, uint32_t verticeId_);
		void queueUpdate();

		Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_); 
	};
} 
