#pragma once 
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <format>

enum StorageUnitEnum : uint8_t 
{ 
	BYTE = 0,
	KB = 1,
	MB = 2,
	GB = 3
};

struct Buffer {
	VkBuffer vkHandle = VK_NULL_HANDLE;
	VkDeviceSize toBytes(uint64_t size_, StorageUnitEnum unit_);
	

	void create(VkDeviceSize size_, StorageUnitEnum unit_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, const char* name_);
	void destroy(); // temporary. Will move into destructor later, once I figure out - how i want my resource managment to be structured
	VkBuffer get() const;
};
