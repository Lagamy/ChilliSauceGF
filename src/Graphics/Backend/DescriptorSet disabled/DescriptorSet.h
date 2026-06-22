// Pointer table for shaders to connect to In Memory GPU Resources 
#pragma once 
#include "BindingLayout.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

struct DescriptorSet { 
	VkDescriptorSet vkHandle = VK_NULL_HANDLE;

	std::vector<BindingLayout> bindingLayouts; // Created from DescriptorSetLayout's entries 
	
	
	void init(); // Allocates mem via bindingLayouts sending a signal + assetId to MemoryManager, and creating DescriptorSet afterwards. Lazy-loads resources if entity had called.  
	void destroy(); 

	void useAssets(); 

	VkDescriptorSet get() const;
}; 
