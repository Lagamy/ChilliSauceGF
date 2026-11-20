#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <string>
#include <vulkan/vulkan_core.h>
#include "Layout.h"
#include "Utilities.h"
#include "DescriptorLayout.h"
#include "Pool.h"
#include "DescriptorPool.h"
#include "DescriptorSetCreateParams.h"

namespace Graphics 
{
struct DescriptorSetLayout 
{
	std::string name;
	uint32_t setIndex; // The set number the shader uses (layout(set = setIndex, ...)): the position of this layout in the pipeline layouts of the pipelines whose shaders declare it, and the index bindDescriptorSet binds its sets at. Chosen by the user, fixed.
	VkDescriptorSetLayout vkHandle = VK_NULL_HANDLE;
	VkShaderStageFlags shaderStages;
	bool dynamic = false; // At least one committed descriptor layout is dynamic (kept up to date by commitSetLayoutUpdate), so sets of this layout take dynamic offsets when bound.
	bool updateOpen = false; // a SetLayoutUpdate of it wasn't committed yet, only one at a time
	bool isBindless = false; 
	uint32_t startingEntityAmount = 0; // For Bindless
	Pool<DescriptorLayout> descriptorLayouts;
	Pool<PoolId> usedByPipelineLayoutIds; 
	uint32_t setsPerPool; // how many sets one pool of this layout holds, chosen by the user (DescriptorSetsPerPool by default)
	std::vector<DescriptorPool> pools; // the sets derived from this layout live in these, each sized for setsPerPool sets. Added by allocateSets when all are full, destroyed in destroy()

	VkDescriptorSetLayout get() const; 
	DescriptorSetLayout(const char* name_, uint32_t setIndex_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_); 
	

	DescriptorLayout& getDescriptorLayout(PoolId id_); 
	
	// Note: the add functions create the descriptor layout flagged pending, SetLayoutUpdate lists it and DescriptorsManager::commitSetLayoutUpdate does the rest (recreating this layout, its sets and the pipeline layouts, which is expensive and waits for the GPU). 
	PoolId addBufferDescriptorLayout(const char* name_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t headroom_ = 0); // binding_ is the binding number of the shader, unique in this layout (throws otherwise). type_: UNIFORM_DESCRIPTOR or STORAGE_DESCRIPTOR. The members of its block are added through the DescriptorLayout, unique only inside this set layout
	PoolId addDynamicDescriptorLayout(const char* name_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_, uint32_t headroom_ = 0); // same, but the block is repeated dynamicCount_ times and one of them is picked per bind
	PoolId addBufferDescriptorLayoutBody(const char* name_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_, uint32_t headroom_); // shared body of the two above, dynamicCount_ 0 = not dynamic
	PoolId addImageDescriptorLayout(const char* name_, uint32_t binding_, DescriptorTypeEnum type_); // TEXTURE_DESCRIPTOR / SAMPLER_DESCRIPTOR, not supported yet (throws) // unique only inside this layout, the descriptor layout and its ReflectionLayout are named "<layout name>/<name_>"
	
	void removeDescriptorLayout(PoolId id_); // removes it and keeps `dynamic` up to date, nothing is recreated (commitSetLayoutUpdate and removeDescriptorSetLayout do that)
	PoolId deriveDescriptorSet(const DescriptorSetCreateParams& rParams_); // rParams_.layoutId is the id of this layout in DescriptorsManager::layouts
	void addPool(); // called by allocateSets
	uint32_t allocateSets(uint32_t count_, VkDescriptorSet* pHandles_); // allocates count_ VkDescriptorSets of this layout from the first pool with room (a new pool if none has) and returns the index of that pool in pools
	void create(); // only call after descriptor descriptor layouts are setted up 
	void recreate(); // destroy + create + recreation of every PipelineLayout (and Pipelines) that uses this layout 
	void destroy();
	void throwIfSetsDerived(const char* action_) const;
	void updateDynamicFlag();
};
}
