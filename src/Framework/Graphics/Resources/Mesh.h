#pragma once
#include "PoolMap.h"
#include "Utilities.h"
#include "Vertex.h"
#include "MemoryEntryId.h"
#include "GPUDataContainer.h"
#include "ReadbackRequest.h"
#include "Layout.h"
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace Graphics
{
	ReflectionLayout& getVerticeLayout(PoolId layoutId); // Forward declaration of function. Implementation is in API
	ReadbackId requestMemoryEntryReadback(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_); // Implementation is in API
	ReadbackDataContainer mapReadback(ReadbackId readbackId_); // Implementation is in API

	/* The mesh keeps no copy of its vertex bytes. Every set writes straight into the memory the entry lives in (a staging range or the CPU_SHARED block), see MemoryManager::getStagingContainer. */
	struct Mesh {
		std::string name;
		PoolId verticeLayoutId;
		uint32_t verticesCount;
		MemoryVisabilityEnum memoryVisability;
		bool readOnly = false;
		std::vector<uint32_t> indices;

		MemoryEntryId vbMemoryUploadId; // Handle to mem entry in the GPUMemoryManager
		MemoryEntryId ibMemoryUploadId;

		/* Writes one member of one vertice. Only valid after queueUpload. Before commitUpload it fills the initial data, after it it's an update (throws if the mesh was queued readOnly). */
		template<typename T>
		void setVerticeMember(PoolId memberId_, uint32_t verticeId_, T data_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			const MemberBlueprint& rMember = rLayout.memberBlueprints[memberId_];
			uint64_t inEntryOffset = verticeId_ * rLayout.size + rMember.firstByteId;

			if(this->memoryVisability == GPU_LOCAL)
			{
				StagingDataContainer container = this->getStagingContainer(inEntryOffset, rMember.size);
				container.repeatCount = this->verticesCount;
				rLayout.setMember(container, memberId_, verticeId_, data_);
			}
			else
			{
				SharedDataContainer container = this->getSharedContainer();
				container.repeatCount = this->verticesCount;
				container.validFirstOffset = inEntryOffset;
				container.validSize = rMember.size;
				rLayout.setMember(container, memberId_, verticeId_, data_);
			}
		}

		/*
			Reading is asynchronous for GPU_LOCAL meshes: the request queues a copy into a readback range, and the member can be read once isReadbackReady(readbackId) is true.
			For CPU_SHARED meshes the request is ready immediately and reads the mapped memory. The caller releases the request with releaseReadback after reading.
		*/
		ReadbackId requestVerticeMemberReadback(PoolId memberId_, uint32_t verticeId_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			const MemberBlueprint& rMember = rLayout.memberBlueprints[memberId_];
			uint64_t inEntryOffset = verticeId_ * rLayout.size + rMember.firstByteId;
			return requestMemoryEntryReadback(this->vbMemoryUploadId, inEntryOffset, rMember.size);
		}

		/* Reads the member a request made with requestVerticeMemberReadback covers. Only valid once the readback is ready. */
		template<typename T>
		T getVerticeMember(ReadbackId readbackId_, PoolId memberId_, uint32_t verticeId_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getMember<T>(container, memberId_, verticeId_);
		}

		void queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, bool readOnly_ = false);
		void commitUpload();

		Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_);

	private:
		StagingDataContainer getStagingContainer(uint64_t inEntryOffset_, uint64_t size_);
		SharedDataContainer getSharedContainer();
	};
}
