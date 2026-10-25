// Pointer table for shaders to connect to In Memory GPU Resources 
#pragma once 
#include "MemoryEntryId.h"
#include "DescriptorBinding.h"
#include "DescriptorPool.h"
#include "Pool.h"
#include <cstdint>
#include <vulkan/vulkan.h>

namespace Graphics 
{
struct DescriptorSet
{ 
	DescriptorPool pool; // its 1 to 1 due to dynamic buffers. 
	VkDescriptorSet vkHandle = VK_NULL_HANDLE;
	Pool<DescriptorBinding> bindings;

	void init(); 
	void destroy(); 

	void create(); 

	VkDescriptorSet get() const;

};
} 
