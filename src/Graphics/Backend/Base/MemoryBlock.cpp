// TODO: Study this more, for more robust memory handling. 
#include "MemoryBlock.h"
#include "Globals.h"

void MemoryBlock::create(VkMemoryRequirements memoryRequirements_, VkMemoryPropertyFlags properties_, const char* name_)
{
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = this->size;
	// Which type of memory to allocate from(device-local VRAM only, host-visible system RAM shared, etc)
	memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memoryRequirements_.memoryTypeBits, properties_); // Index of memory type on Physical Device that has required bit flags for this buffer

	// Allocate memory to VkDeviceMemory 
	VkResult result = vkAllocateMemory(Demo::renderer.mainDevice.logicalDevice, &memAllocInfo, nullptr, &this->vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(std::format("Failed to allocate {} Memory Block!", name_));
	}
}

void MemoryBlock::destroy()
{
	vkFreeMemory(Engine::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
}

VkDeviceMemory MemoryBlock::get() const
{
	return this->vkHandle;
}

uint32_t MemoryBlock::findMemoryTypeIndex(uint32_t allowedTypes_, VkMemoryPropertyFlags properties_)
{
	// Note: _allowedTypes - filter by hardware compatibility, so its GPU's driver responsability to give you that. DO NOT pass anything outside of VkMemoryRequirements.memoryTypeBits in it.  

	// The physical device exposes several memory heaps, each with different properties(device-local, host-visible, etc.) vkGetPhysicalDeviceMemoryProperties retrieves all of them.
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(Demo::renderer.mainDevice.physicalDevice, &memoryProperties);

	// When allocating memory for a buffer or image, Vulkan gives us a bitmask(allowedTypes_) describing which memory types can be used. 
	// So we need to loop through all available memory types on this device and finds one whose bit is set in allowedTypes_ and whose property flags include all of the desired properties_.

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		// bitshift 1 by i each iteration to go Type by Type through memoryProperties.memoryTypes, and check if it is equal to least 1 type of allowedTypes_ via logical "and"(&);
		if ((allowedTypes_ & (1 << i)) // Index of memory type must match corresponding bit in allowedTypes 
			&& (memoryProperties.memoryTypes[i].propertyFlags & properties_) == properties_) // filter out memoryTypes.propertyFlags to include only flags from properties_ if they exist in memoryTypes.propertyFlags, and check if all _property flags are there.  
		{
			// This memory type is valid, so retun its index 
			return i;
		}
	}
}


