#pragma once 
#include "Utilities.h"
#include <string> 
#include <vulkan/vulkan.h>

namespace Graphics {
	struct UploadEntry 
	{ 
		std::string name; 
		const void* data; 
		uint32_t pageId; // Used only in Dynamically allocated Entries 
		size_t inBufferFirstByte = 0; 
		VkDeviceSize size = 0;
		VkDeviceSize inGPUFirstByte = 0; 
		BufferTypeEnum bufferType; 
		PoolId bufferId; // Needed only if in dynamic allocator 

    	UploadEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_, VkDeviceSize currentBuffSize_) : name(name_), data(data_), size(size_), bufferType(bufferType_), inBufferFirstByte(currentBuffSize_) {}; 
    	UploadEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_) : name(name_), data(data_), size(size_), bufferType(bufferType_) {}; 
		bool inGPU = false; // for Dynamic allocators. 
	}; 
}


