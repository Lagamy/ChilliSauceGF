#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <string>
#include <vulkan/vulkan_core.h>
#include "Layout.h"
#include "Utilities.h"
#include "DescriptorBlueprint.h"
#include "Pool.h"

namespace Graphics 
{
struct DescriptorSetLayout 
{
	std::string name;
	VkDescriptorSetLayout vkHandle = VK_NULL_HANDLE;
	bool isBindless; 
	uint32_t startingEntityAmount; // For Bindless
	Pool<DescriptorBlueprint> descriptorBlueprints;
	Pool<PoolId> usedByPipelineLayoutIds; 

	VkDescriptorSetLayout get() const; 
	DescriptorSetLayout(DescriptorTypeEnum descriptorType_, VkShaderStageFlags shaderStages_); 
	

	DescriptorBlueprint& getDescriptorBlueprint(PoolId id_); 
	
	// Note: those functions involve Vulkan Pipelines recreation(Which are expensive). Only trigger them in your engines Editor context. Not runtime. 
	PoolId addDescriptorBlueprint(const char* name_); // name for debug porpuses
	
	void removeDescriptorBlueprint(PoolId id_); 
	PoolId deriveDescriptorSet(); 
	void create(); // only call after descriptor blueprints are setted up 
	void destroy();
};
}