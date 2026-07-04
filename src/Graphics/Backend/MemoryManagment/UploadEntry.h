#pragma once 
#include <string> 
#include <vulkan/vulkan.h>

struct UploadEntry 
{ 
	std::string name; 
	const void* data; 
	size_t heapStartingByte = 0; 
	VkDeviceSize size; 
    UploadEntry(const char* name_, const void* data_, VkDeviceSize size_, VkDeviceSize currentBuffSize_) : name(name_), data(data_), size(size_), heapStartingByte(currentBuffSize_) {}; 
};

