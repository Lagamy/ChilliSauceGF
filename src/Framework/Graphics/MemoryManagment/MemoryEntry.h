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
		

		// For Upload(only for Update when static)
		uint64_t uploadBatchId = 0;
		bool isPendingUpload();
		bool isPendingUploadStatic();
		void upload(const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);

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


