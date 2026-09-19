#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include "Semaphore.h"
#include "PassId.h"
#include <string>
#include <vulkan/vulkan.h>

namespace Graphics {
	void uploadEntryCMDs(VkCommandBuffer& cmdBuffer_); 
	struct UploadEntry 
	{ 
		std::string name; 
		VkDeviceSize size = 0;
		VkDeviceSize inGPUFirstByte = 0; 
		BufferTypeEnum bufferType;
		bool isForDynamic; 

		// For Upload(only for Update when static)
		PoolId isUploadedFenceId; 
		PoolId isUploadedSemaphoreId; 
		PassId uploadPassId;
		bool isPendingUpload(); 
		void upload(const void* data_, uint64_t size_, uint64_t offset_);
		Semaphore& getIsFinishedSemaphore();
		
		/* For Static */
		const void* data; 
		size_t inBufferFirstByte = 0; 
		UploadEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_, VkDeviceSize currentBuffSize_);
		/*-----------*/

		/* For Dynamic */
		uint32_t pageId;
		PoolId bufferId;
		MemoryVisabilityEnum memoryVisability; 
		UploadEntry(const char* name_, VkDeviceSize size_,  BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, uint32_t pageId_, PoolId bufferId_);
		~UploadEntry();
		/*------------*/

	};
}


