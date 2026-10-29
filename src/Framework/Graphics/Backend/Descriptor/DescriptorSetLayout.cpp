#include "DescriptorSetLayout.h"
#include "Api.h"
#include <format>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics 
{
	DescriptorSetLayout::DescriptorSetLayout(const char* name_, DescriptorTypeEnum descriptorType_, VkShaderStageFlags shaderStages_) : name(name_), descriptorType(descriptorType_), shaderStages(shaderStages_)
	{
		if(descriptorType_ > STORAGE_DESCRIPTOR)
		{
			throw std::runtime_error("Descriptor Set Layout: Only UNIFORM_DESCRIPTOR and STORAGE_DESCRIPTOR are supported.");
		}
	}

	VkDescriptorSetLayout DescriptorSetLayout::get() const
	{
		return this->vkHandle;
	}

    DescriptorBlueprint& DescriptorSetLayout::getDescriptorBlueprint(PoolId id_)
    {
		return this->descriptorBlueprints[id_];
    } 

	/* A derived set has one descriptor per blueprint slot and a VkDescriptorSet allocated from this layout's handle, so the blueprints can't change underneath it. */
	void DescriptorSetLayout::throwIfSetsDerived(const char* action_) const
	{
		for(uint32_t i = 0; i < this->descriptorBlueprints.objects.size(); i++)
		{
			if(this->descriptorBlueprints.alive[i] && !this->descriptorBlueprints.objects[i].derivedDescriptors.empty())
			{
				throw std::runtime_error(std::format("Descriptor Set Layout {}: Can't {} a layout that already has derived Descriptor Sets.", this->name, action_));
			}
		}
	}
	
	/*
		The blueprint gets its own uniform / storage ReflectionLayout (the same way addVerticeLayout makes one), the members of the descriptor are added to it through the blueprint.
		ReflectionSystem keeps one name map per buffer type for all set layouts, so both names are prefixed with the layout name: two layouts can then both have a blueprint called "Camera".
	*/
	PoolId DescriptorSetLayout::addDescriptorBlueprint(const char* name_)
    {
		this->throwIfSetsDerived("add a blueprint to");

		BufferTypeEnum bufferType = static_cast<BufferTypeEnum>(UNIFORM_BUFFER + this->descriptorType);
		std::string fullName = std::format("{}/{}", this->name, name_);
		PoolId reflectionLayoutId = getReflectionSystem().layoutsPerBufferType[bufferType - 1].add(fullName.c_str()); // - 1 due to INDEX layout not existing
		PoolId blueprintId = this->descriptorBlueprints.add(fullName.c_str(), bufferType, reflectionLayoutId);

        if(this->vkHandle != VK_NULL_HANDLE)
        {
            this->recreate();
        }
		return blueprintId;
    }

	void DescriptorSetLayout::removeDescriptorBlueprint(PoolId id_)
    {
		this->throwIfSetsDerived("remove a blueprint from");

		/* The blueprint owns its ReflectionLayout, so the layout goes with it. Read the ids before remove(), which invalidates id_. */
		const DescriptorBlueprint& rBlueprint = this->descriptorBlueprints[id_];
		getReflectionSystem().layoutsPerBufferType[rBlueprint.bufferType - 1].remove(rBlueprint.layoutId); // - 1 due to INDEX layout not existing
		this->descriptorBlueprints.remove(id_);
		if(this->vkHandle != VK_NULL_HANDLE)
        {
            this->recreate();
        }
    } 

	/*
		Creates the set and one Descriptor per blueprint slot. Descriptor slot i is binding i, so a removed blueprint slot still gets a descriptor.
		Those are removed only after every slot was added: a removed slot goes to the Pool's free list and the next add would take it, shifting the index of every later descriptor.
	*/
	PoolId DescriptorSetLayout::deriveDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_)
	{
		DescriptorsManager& rDescriptorsManager = getDescriptorsManager();
		PoolId setId = rDescriptorsManager.descriptorSet.add(layoutId_, allocatorType_);
		DescriptorSet& rSet = rDescriptorsManager.descriptorSet[setId];

		std::vector<PoolId> deadSlotDescriptorIds;
		for(uint32_t i = 0; i < this->descriptorBlueprints.allocatedSize(); i++)
		{
			PoolId blueprintId = PoolId(i, this->descriptorBlueprints.generation[i]);
			PoolId descriptorId = rSet.descriptors.add(layoutId_, memoryVisability_, setId, blueprintId);
			if(this->descriptorBlueprints.alive[i])
			{
				this->descriptorBlueprints.getInternal(i).derivedDescriptors.emplace_back(setId);
			}
			else
			{
				deadSlotDescriptorIds.emplace_back(descriptorId);
			}
		}

		for(const PoolId& rDescriptorId : deadSlotDescriptorIds)
		{
			rSet.descriptors.remove(rDescriptorId);
		}
		return setId;
	}

	/* Binding numbers are the blueprint slot indices. They may be sparse, Vulkan only needs them to match the shader. One descriptor per binding, so descriptorCount is 1. */
	void DescriptorSetLayout::create()
    {
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.reserve(this->descriptorBlueprints.size());
		for(uint32_t i = 0; i < this->descriptorBlueprints.allocatedSize(); i++)
		{
			if(!this->descriptorBlueprints.alive[i])
			{
				continue;
			}

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = i;
			binding.descriptorType = descriptorTypes[this->descriptorType];
			binding.descriptorCount = 1;
			binding.stageFlags = this->shaderStages;
			binding.pImmutableSamplers = nullptr;
			bindings.emplace_back(binding);
		}

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = bindings.size();
		layoutCreateInfo.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(getMainDevice().logicalDevice, &layoutCreateInfo, nullptr, &this->vkHandle);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to create Descriptor Set Layout {}!", this->name));
		}
    }

	/* A PipelineLayout copies the set layout handles when it is created, so every layout that uses this one has to be rebuilt (and with it its pipelines). Nothing waits for the GPU here. */
	void DescriptorSetLayout::recreate()
	{
		this->destroy();
		this->create();
		for(uint32_t i = 0; i < this->usedByPipelineLayoutIds.allocatedSize(); i++)
		{
			if(this->usedByPipelineLayoutIds.alive[i])
			{
				getPipelinesManager().pipelineLayouts[this->usedByPipelineLayoutIds.getInternal(i)].recreate();
			}
		}
	}

	void DescriptorSetLayout::destroy()
    {
        if(this->vkHandle != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(getMainDevice().logicalDevice, this->vkHandle, nullptr); 
			this->vkHandle = VK_NULL_HANDLE;
        }
    }
}
