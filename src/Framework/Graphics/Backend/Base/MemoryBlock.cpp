// TODO: Study this more, for more robust memory handling. 
#include "MemoryBlock.h"
#include "Api.h"
#include "PoolId.h"
#include "Utilities.h"

namespace Graphics
{


VkResult MemoryBlock::createInternal(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memPropertyIndex_)
{
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = size_;
	// Which type of memory to allocate from(device-local VRAM only, host-visible system RAM shared, etc)
	memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memReqsSpan_, memPropertyIndex_, false); // Index of memory type on Physical Device that has required bit flags for this buffer
	this->size = size_; 

	// Allocate memory to VkDeviceMemory 
	return vkAllocateMemory(getMainDevice().logicalDevice, &memAllocInfo, nullptr, &this->vkHandle);
}

void MemoryBlock::createForStaticOrStaging(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memPropertyIndex_, MemoryVisabilityEnum memVisability_, bool isStaging_)
{
	this->memVisability = memVisability_; 
	this->isStaging = isStaging_; 
	VkResult result = createInternal(size_, memReqsSpan_, memPropertyIndex_); 
	#ifdef ENGINE_DEBUG
	if (result != VK_SUCCESS)
	{
		if(isStaging_)
		{
			throw std::runtime_error("Failed to allocate Memory Block! For Staging uploads in Static Allocator.");
		}
		else
		{
			throw std::runtime_error(std::format("Failed to allocate Memory Block! For {} uploads in Static Allocator.", MemVisabilityTypeToName[memVisability_]));
		}
	}
	#endif
}


void MemoryBlock::create(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memPropertyIndex_, MemoryVisabilityEnum memVisability_, uint32_t pageUpperBound_, PoolId memBlockId_)
{
	this->memBlockId = memBlockId_;
	this->pageUpperBound = pageUpperBound_; 
	this->memVisability = memVisability_;

	VkResult result = createInternal(size_, memReqsSpan_, memPropertyIndex_); 
	#ifdef ENGINE_DEBUG
	if (result != VK_SUCCESS)
	{
		
		throw std::runtime_error(std::format("Failed to allocate Memory Block! For MemoryBlock id: {} generation: {}. Inside of {} Page {}.",  this->memBlockId.id, this->memBlockId.generation, MemVisabilityTypeToName[this->memVisability], this->pageUpperBound));
	}
	#endif
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

uint32_t MemoryBlock::findMemoryTypeIndex(std::span<VkMemoryRequirements> memReqsSpan_, VkMemoryPropertyFlags properties_,  bool isStatic)
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

	if(isStatic)
	{
		throw std::runtime_error(std::format("Failed to find suitable memory type for Static Allocator.")); 
	}
	else
	{
		if(this->isStaging)
		{
			throw std::runtime_error(std::format("Failed to find suitable memory type for MemoryBlock id: {} generation: {}. Inside of Staging Page {}.",  this->memBlockId.id, this->memBlockId.generation, this->pageUpperBound)); 
		}
		else
		{
			throw std::runtime_error(std::format("Failed to find suitable memory type for MemoryBlock id: {} generation: {}. Inside of {} Page {}.",  this->memBlockId.id, this->memBlockId.generation, MemVisabilityTypeToName[this->memVisability], this->pageUpperBound)); 
		}
	}
}

MemoryBlock::~MemoryBlock()
{
	this->destroy(); 
} 
}
