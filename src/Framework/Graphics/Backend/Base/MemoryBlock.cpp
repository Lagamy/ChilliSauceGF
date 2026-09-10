// TODO: Study this more, for more robust memory handling. 
#include "MemoryBlock.h"
#include "Api.h"
#include <string>

namespace Graphics
{
void MemoryBlock::create(size_t size_, StorageUnitEnum unit_, std::span<VkMemoryRequirements> memReqsSpan_, VkMemoryPropertyFlags properties_, const char* context_)
{
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = this->toBytes(size_, unit_);
	// Which type of memory to allocate from(device-local VRAM only, host-visible system RAM shared, etc)
	memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memReqsSpan_, properties_, context_); // Index of memory type on Physical Device that has required bit flags for this buffer
	

	// Allocate memory to VkDeviceMemory 
	VkResult result = vkAllocateMemory(getMainDevice().logicalDevice, &memAllocInfo, nullptr, &this->vkHandle);
	
	#ifdef ENGINE_DEBUG
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(std::format("Failed to allocate {} Memory Block!", context_));
	}
	#endif

	this->size = size_; 
}


void MemoryBlock::destroy()
{
	if (this->vkHandle != VK_NULL_HANDLE)
	{
		vkFreeMemory(getMainDevice().logicalDevice, this->vkHandle, nullptr);
		this->vkHandle = VK_NULL_HANDLE;
	}
}


VkDeviceMemory MemoryBlock::get() const
{
	return this->vkHandle;
}

uint32_t MemoryBlock::findMemoryTypeIndex(std::span<VkMemoryRequirements> memReqsSpan_, VkMemoryPropertyFlags properties_, const char* context_)
{
	// Note: _allowedTypes - filter by hardware compatibility, so its GPU's driver responsability to give you that. DO NOT pass anything outside of VkMemoryRequirements.memoryTypeBits in it.  

	// The physical device exposes several memory heaps, each with different properties(device-local, host-visible, etc.) vkGetPhysicalDeviceMemoryProperties retrieves all of them.
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(getMainDevice().physicalDevice, &memoryProperties);

	// When allocating memory for a buffer or image, Vulkan gives us a bitmask(allowedTypes_) describing which memory types can be used. 
	// So we need to loop through all available memory types on this device and finds one whose bit is set in allowedTypes_ and whose property flags include all of the desired properties_.

	for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		
		uint32_t allowedTypes = ~0; // 0xFFFFFFFF, basically - invert 0. All bits are on. 


		for (auto& rReq : memReqsSpan_) { 
        	allowedTypes &= rReq.memoryTypeBits; // filter to include only bits that intersect 
		}

		// bitshift 1 by i each iteration to go Type by Type through memoryProperties.memoryTypes, and check if it is equal to least 1 type of allowedTypes_ via logical "and"(&);
		if ((allowedTypes & (1 << i)) // Index of memory type must match corresponding bit in allowedTypes 
			&& (memoryProperties.memoryTypes[i].propertyFlags & properties_) == properties_) // filter out memoryTypes.propertyFlags to include only flags from properties_ if they exist in memoryTypes.propertyFlags, and check if all _property flags are there.  
		{
			// This memory type is valid, so retun its index 
			return i;
		}
	}
	std::stringstream errorMessageStream; 
	errorMessageStream << context_ << ": Failed to find suitable memory type."; 
	throw std::runtime_error(errorMessageStream.str());
}

VkDeviceSize MemoryBlock::toBytes(size_t size_, StorageUnitEnum unit_)
{
	return size_ << (10 * static_cast<uint8_t>(unit_));
}

MemoryBlock::MemoryBlock(size_t size_, StorageUnitEnum unit_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memoryTypeIndex_, const char* context_)
{
	this->create(size_, unit_, memReqsSpan_, memoryTypeIndex_, context_);
}

MemoryBlock::~MemoryBlock()
{
	this->destroy(); 
} 
}
