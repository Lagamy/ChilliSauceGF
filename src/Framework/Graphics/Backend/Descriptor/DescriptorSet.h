// Pointer table for shaders to connect to In Memory GPU Resources 
#pragma once 
#include "MemoryEntryId.h"
#include "DescriptorBinding.h"
#include "Descriptor.h"
#include "DescriptorPool.h"
#include "Pool.h"
#include "Utilities.h"
#include <cstdint>
#include <vulkan/vulkan.h>

namespace Graphics 
{
struct DescriptorSet
{
	PoolId setLayout; 
	DescriptorTypeEnum descriptorType; // inhereted from Pool  
	DescriptorPool pool; // its 1 to 1 due to dynamic buffers. That way the logic is much simpler. 
	VkDescriptorSet vkHandle = VK_NULL_HANDLE;
	Pool<DescriptorBinding> bindings;
	Pool<Descriptor> descriptors; // per binding

	VkDescriptorSet get() const;
	bool descriptorsMemoryAllocated = false; 

	DescriptorSet(PoolId setLayout_) : setLayout(setLayout_){}; 
	void setDescriptorMemberValue(PoolId descriptorId_, PoolId memberId_); 	
	
	void create(AllocatorTypeEnum bufferAllocatorType_); 
	void destroy();
};
} 
