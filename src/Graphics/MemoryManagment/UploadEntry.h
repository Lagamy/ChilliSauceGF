#pragma once 
#include "Utilities.h"
#include <string> 
#include <vulkan/vulkan.h>

namespace Graphics {
	struct UploadEntry 
	{ 
		std::string name; 
		const void* data; 
		size_t inBufferFirstByte = 0; 
		VkDeviceSize size = 0;
		VkDeviceSize inGPUFirstByte = 0; 
		BufferTypeEnum bufferType; 
    	UploadEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_, VkDeviceSize currentBuffSize_) : name(name_), data(data_), size(size_), bufferType(bufferType_), inBufferFirstByte(currentBuffSize_) {}; 
		bool inGPU = false; // for Dynamic allocators. 
	}; 
}


