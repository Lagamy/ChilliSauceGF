#pragma once 
#include "PoolId.h"
#include "StagingData.h"
#include "Utilities.h"
#include "Semaphore.h"
#include "PassId.h"
#include <string>
#include <vulkan/vulkan.h>

namespace Graphics {
	struct MemoryEntry 
	{ 
		std::string name; 
		VkDeviceSize size = 0;
		BufferTypeEnum bufferType;
		uint32_t pageId; // Also needed for static, so I can optimize the updates
		StagingData stagingData;
		bool isForDynamic = false;
		bool readOnly = false; // Declared at creation. A readOnly entry can be locked with MemoryManager::lockEntry.
		bool locked = false; // Set by MemoryManager::lockEntry. Writes (updates, staging and shared containers) throw from then on. Readbacks and removal stay allowed.

		// For Uniform / Storage 
		PoolId descriptorSet;

		// For Upload(only for Update when static)
		uint64_t uploadBatchId = 0;
		bool isPendingUpload();
		uint64_t readbackBatchId = 0; // Batch of the latest readback copy reading this entry's buffer. Update and removal wait on it.
		bool isPendingReadback();
		bool awaitingCommit = false; // Reserved with MemoryManager::reserveEntry, copy not queued yet. stagingData is the whole-entry range.
		void upload(const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);
		void queueStagedCopy(const StagingData& rStagingData_, uint64_t inEntryOffset_, uint64_t size_);
		uint64_t queueReadbackCopy(const StagingData& rStagingData_, uint64_t inEntryOffset_, uint64_t size_);

		/* For Static */
		const void* data; 
		size_t inBufferFirstByte = 0; 
		VkDeviceSize inGPUFirstByte = 0; // Absolute firstByte(Buffer offset + in buffer First Byte. ) 
		MemoryEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, VkDeviceSize currentBuffSize_);  
		/*-----------*/

		/* For Dynamic */
		PoolId bufferId;
		PoolId memoryBlockId;
		MemoryVisabilityEnum memoryVisability;
		
		MemoryEntry(const char* name_, VkDeviceSize size_,  BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, uint32_t pageId_, PoolId memoryBlockId_, PoolId bufferId_);
		/*------------*/

	};
}


