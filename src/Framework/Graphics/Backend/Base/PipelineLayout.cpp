#include "PipelineLayout.h"
#include "Api.h"

namespace Graphics
{
/*
	The descriptor limits of the spec are per pipeline layout, not per set: the total of uniform (and storage) buffer descriptors over all its sets has to fit maxDescriptorSetUniformBuffers (maxDescriptorSetStorageBuffers), the dynamic ones maxDescriptorSet...Dynamic, and the descriptors visible to one stage have to fit maxPerStageDescriptor...Buffers and maxPerStageResources (buffers are the only descriptor types supported so far). Dynamic descriptors count in both the plain and the dynamic total. Checked here because vkCreatePipelineLayout with a breach is invalid usage, and the validation message doesn't name the set layouts.
*/
void PipelineLayout::validateLimits() const
{
	const VkPhysicalDeviceLimits& rLimits = getMainDevice().properties.limits;
	if(this->descriptorSetLayoutIds.size() > rLimits.maxBoundDescriptorSets)
	{
		throw std::runtime_error(std::format("Pipeline Layout: {} descriptor sets (set indices up to {}), the device allows {} bound at once.", this->descriptorSetLayoutIds.size(), this->descriptorSetLayoutIds.size() - 1, rLimits.maxBoundDescriptorSets));
	}

	struct Stage { VkShaderStageFlagBits flag; const char* name; };
	const Stage stages[] = { { VK_SHADER_STAGE_VERTEX_BIT, "vertex" }, { VK_SHADER_STAGE_FRAGMENT_BIT, "fragment" }, { VK_SHADER_STAGE_COMPUTE_BIT, "compute" } };
	uint32_t uniformCount = 0;
	uint32_t storageCount = 0;
	uint32_t dynamicUniformCount = 0;
	uint32_t dynamicStorageCount = 0;
	uint32_t uniformPerStage[std::size(stages)] = {};
	uint32_t storagePerStage[std::size(stages)] = {};
	for(const PoolId& layoutId : this->descriptorSetLayoutIds)
	{
		if(layoutId == UninitializedPoolId)
		{
			continue;
		}
		DescriptorSetLayout& rSetLayout = getDescriptorsManager().layouts[layoutId];
		for(uint32_t i = 0; i < rSetLayout.descriptorLayouts.allocatedSize(); i++)
		{
			if(!rSetLayout.descriptorLayouts.alive[i])
			{
				continue;
			}
			DescriptorLayout& rDescriptorLayout = rSetLayout.descriptorLayouts.objects[i];
			bool uniform = rDescriptorLayout.type == UNIFORM_DESCRIPTOR;
			(uniform ? uniformCount : storageCount)++;
			if(rDescriptorLayout.dynamic)
			{
				(uniform ? dynamicUniformCount : dynamicStorageCount)++;
			}
			for(uint32_t stage = 0; stage < std::size(stages); stage++)
			{
				if(rSetLayout.shaderStages & stages[stage].flag)
				{
					(uniform ? uniformPerStage : storagePerStage)[stage]++;
				}
			}
		}
	}

	if(uniformCount > rLimits.maxDescriptorSetUniformBuffers)
	{
		throw std::runtime_error(std::format("Pipeline Layout: {} uniform descriptors over all its sets, the device allows {}.", uniformCount, rLimits.maxDescriptorSetUniformBuffers));
	}
	if(storageCount > rLimits.maxDescriptorSetStorageBuffers)
	{
		throw std::runtime_error(std::format("Pipeline Layout: {} storage descriptors over all its sets, the device allows {}.", storageCount, rLimits.maxDescriptorSetStorageBuffers));
	}
	if(dynamicUniformCount > rLimits.maxDescriptorSetUniformBuffersDynamic)
	{
		throw std::runtime_error(std::format("Pipeline Layout: {} dynamic uniform descriptors over all its sets, the device allows {}.", dynamicUniformCount, rLimits.maxDescriptorSetUniformBuffersDynamic));
	}
	if(dynamicStorageCount > rLimits.maxDescriptorSetStorageBuffersDynamic)
	{
		throw std::runtime_error(std::format("Pipeline Layout: {} dynamic storage descriptors over all its sets, the device allows {}.", dynamicStorageCount, rLimits.maxDescriptorSetStorageBuffersDynamic));
	}
	for(uint32_t stage = 0; stage < std::size(stages); stage++)
	{
		if(uniformPerStage[stage] > rLimits.maxPerStageDescriptorUniformBuffers)
		{
			throw std::runtime_error(std::format("Pipeline Layout: {} uniform descriptors visible to the {} stage, the device allows {}.", uniformPerStage[stage], stages[stage].name, rLimits.maxPerStageDescriptorUniformBuffers));
		}
		if(storagePerStage[stage] > rLimits.maxPerStageDescriptorStorageBuffers)
		{
			throw std::runtime_error(std::format("Pipeline Layout: {} storage descriptors visible to the {} stage, the device allows {}.", storagePerStage[stage], stages[stage].name, rLimits.maxPerStageDescriptorStorageBuffers));
		}
		if(uniformPerStage[stage] + storagePerStage[stage] > rLimits.maxPerStageResources)
		{
			throw std::runtime_error(std::format("Pipeline Layout: {} resources visible to the {} stage, the device allows {}.", uniformPerStage[stage] + storagePerStage[stage], stages[stage].name, rLimits.maxPerStageResources));
		}
	}
}

void PipelineLayout::create()
{
	this->validateLimits();
	std::vector<VkDescriptorSetLayout> descriptorLayouts; 
	descriptorLayouts.reserve(this->descriptorSetLayoutIds.size());
	for(const PoolId& layoutId : this->descriptorSetLayoutIds)
	{
		if(layoutId == UninitializedPoolId)
		{
			descriptorLayouts.emplace_back(getPipelinesManager().getEmptySetLayout());
			continue;
		}
		descriptorLayouts.emplace_back(getDescriptorsManager().layouts[layoutId].get()); 
	}

	/*
		One range per use, every use has its own shader stages (a pipeline layout can have only one range per stage, PipelinesManager::addPushConstantLayoutToPipeline throws on overlapping stages).
		The offsets come from PushConstantStorage, which also throws when the ranges together are more than maxPushConstantsSize.
	*/
	getMemoryManager().pushConstantStorage.place(this->pushConstantUses);
	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.reserve(this->pushConstantUses.size());
	for(const PushConstantUse& rUse : this->pushConstantUses)
	{
		VkPushConstantRange range = {};
		range.stageFlags = rUse.stages;
		range.offset = rUse.offset;
		range.size = static_cast<uint32_t>(getPipelinesManager().pushConstantLayouts[rUse.layoutId].makeShape().blockSize);
		pushConstantRanges.emplace_back(range);
	}

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = descriptorLayouts.size();
	layoutCreateInfo.pSetLayouts = descriptorLayouts.data();
	layoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
	layoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

	VkResult result = vkCreatePipelineLayout(getMainDevice().logicalDevice, &layoutCreateInfo, nullptr, &vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}
}

/*
	A pipeline keeps working after its layout object is destroyed, but a layout can't be destroyed while submitted commands still use it, and a pipeline can't be destroyed while commands that bind it are pending.
	Nothing here waits for the GPU: the caller has to make sure nothing submitted uses the layout or its pipelines (editor context only).
*/
void PipelineLayout::recreate()
{
	this->destroy();
	this->create();
	for(const PoolId& rPipelineId : this->usedByGraphicsPipelineIds)
	{
		GraphicsPipeline& rPipeline = getPipelinesManager().graphicsPipelines[rPipelineId];
		rPipeline.destroy();
		rPipeline.create();
	}
}

void PipelineLayout::destroy()
{
	if (vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(getMainDevice().logicalDevice, vkHandle, nullptr);
		this->vkHandle = VK_NULL_HANDLE;
	}
}

VkPipelineLayout PipelineLayout::get() const
{
	return this->vkHandle;
}

const PushConstantUse& PipelineLayout::getPushConstantUse(PoolId pushConstantLayoutId_) const
{
	for(const PushConstantUse& rUse : this->pushConstantUses)
	{
		if(rUse.layoutId == pushConstantLayoutId_)
		{
			return rUse;
		}
	}
	throw std::runtime_error("Pipeline Layout: the push constant layout is not attached to the pipeline.");
}
}
