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
	std::string name; // Prefix of the names of its blueprints and their ReflectionLayouts.
	VkDescriptorSetLayout vkHandle = VK_NULL_HANDLE;
	DescriptorTypeEnum descriptorType; // Every blueprint of the layout is a descriptor of this type.
	VkShaderStageFlags shaderStages;
	bool isBindless = false; 
	uint32_t startingEntityAmount = 0; // For Bindless
	Pool<DescriptorBlueprint> descriptorBlueprints;
	Pool<PoolId> usedByPipelineLayoutIds; 

	VkDescriptorSetLayout get() const; 
	DescriptorSetLayout(const char* name_, DescriptorTypeEnum descriptorType_, VkShaderStageFlags shaderStages_); 
	

	DescriptorBlueprint& getDescriptorBlueprint(PoolId id_); 
	
	// Note: those functions involve Vulkan Pipelines recreation(Which are expensive). Only trigger them in your engines Editor context. Not runtime. 
	// Also they don't wait for the GPU: the caller must guarantee that nothing submitted still uses the layout. 
	PoolId addDescriptorBlueprint(const char* name_); // unique only inside this layout, the blueprint and its ReflectionLayout are named "<layout name>/<name_>"
	
	void removeDescriptorBlueprint(PoolId id_); 
	PoolId deriveDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_); // layoutId_ is the id of this layout in DescriptorsManager::layouts
	void create(); // only call after descriptor blueprints are setted up 
	void recreate(); // destroy + create + recreation of every PipelineLayout (and Pipelines) that uses this layout 
	void destroy();

	private:
	void throwIfSetsDerived(const char* action_) const;
};
}
