#include "DescriptorSetLayout.h"
#include "Api.h"
#include <array>
#include <format>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics 
{
	DescriptorSetLayout::DescriptorSetLayout(const char* name_, uint32_t setIndex_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_) : name(name_), setIndex(setIndex_), shaderStages(shaderStages_), setsPerPool(setsPerPool_)
	{
		if(setsPerPool_ == 0)
		{
			throw std::runtime_error(std::format("Descriptor Set Layout {}: setsPerPool_ has to be at least 1.", name_));
		}
	}

	VkDescriptorSetLayout DescriptorSetLayout::get() const
	{
		return this->vkHandle;
	}

    DescriptorLayout& DescriptorSetLayout::getDescriptorLayout(PoolId id_)
    {
		return this->descriptorLayouts[id_];
    } 

	/* A derived set has one descriptor per descriptor layout slot and a VkDescriptorSet allocated from this layout's handle, so the descriptor layouts can't change underneath it. */
	void DescriptorSetLayout::throwIfSetsDerived(const char* action_) const
	{
		for(uint32_t i = 0; i < this->descriptorLayouts.objects.size(); i++)
		{
			if(this->descriptorLayouts.alive[i] && !this->descriptorLayouts.objects[i].derivedDescriptors.empty())
			{
				throw std::runtime_error(std::format("Descriptor Set Layout {}: Can't {} a layout that already has derived Descriptor Sets.", this->name, action_));
			}
		}
	}
	
	PoolId DescriptorSetLayout::addImageDescriptorLayout(const char* name_, uint32_t binding_, DescriptorTypeEnum type_)
	{
		throw std::runtime_error(std::format("Descriptor Set Layout {}: Image descriptor layout {} is not supported yet.", this->name, name_));
	}

	PoolId DescriptorSetLayout::addBufferDescriptorLayout(const char* name_, uint32_t binding_, DescriptorTypeEnum type_)
	{
		return this->addBufferDescriptorLayoutBody(name_, binding_, type_, 0);
	}

	PoolId DescriptorSetLayout::addDynamicDescriptorLayout(const char* name_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_)
	{
		if(dynamicCount_ == 0)
		{
			throw std::runtime_error(std::format("Descriptor Set Layout {}: Dynamic buffer descriptor layout {} needs at least one dynamic unit.", this->name, name_));
		}
		return this->addBufferDescriptorLayoutBody(name_, binding_, type_, dynamicCount_);
	}

	/* The members of the block are added afterwards through the DescriptorLayout. */
	PoolId DescriptorSetLayout::addBufferDescriptorLayoutBody(const char* name_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_)
    {
		if(type_ != UNIFORM_DESCRIPTOR && type_ != STORAGE_DESCRIPTOR)
		{
			throw std::runtime_error(std::format("Descriptor Set Layout {}: Buffer descriptor layout {} can only be UNIFORM_DESCRIPTOR or STORAGE_DESCRIPTOR.", this->name, name_));
		}
		this->throwIfSetsDerived("add a descriptor layout to");

		/* Two bindings with the same number make an invalid VkDescriptorSetLayout, so it is caught here with the names. */
		for(uint32_t i = 0; i < this->descriptorLayouts.allocatedSize(); i++)
		{
			if(this->descriptorLayouts.alive[i] && this->descriptorLayouts.objects[i].binding == binding_)
			{
				throw std::runtime_error(std::format("Descriptor Set Layout {}: Binding {} of descriptor layout {} is already used by {}.", this->name, binding_, name_, this->descriptorLayouts.objects[i].name));
			}
		}

		BufferTypeEnum bufferType = descriptorBufferTypes[type_];
		PoolId descriptorLayoutId = this->descriptorLayouts.add(name_, binding_, type_, dynamicCount_, bufferType);
		if(dynamicCount_ > 0)
		{
			this->dynamic = true;
		}

        if(this->vkHandle != VK_NULL_HANDLE)
        {
            this->recreate();
        }
		return descriptorLayoutId;
    }

	void DescriptorSetLayout::removeDescriptorLayout(PoolId id_)
    {
		this->throwIfSetsDerived("remove a descriptor layout from");

		this->descriptorLayouts.remove(id_);
		this->dynamic = false;
		for(uint32_t i = 0; i < this->descriptorLayouts.allocatedSize(); i++)
		{
			if(this->descriptorLayouts.alive[i] && this->descriptorLayouts.objects[i].dynamic)
			{
				this->dynamic = true;
				break;
			}
		}
		if(this->vkHandle != VK_NULL_HANDLE)
        {
            this->recreate();
        }
    } 

	/*
		Creates the set and one Descriptor per descriptor layout slot. Descriptor slot i belongs to descriptor layout slot i (its binding is DescriptorLayout::binding), so a removed descriptor layout slot still gets a descriptor.
		Those are removed only after every slot was added: a removed slot goes to the Pool's free list and the next add would take it, shifting the index of every later descriptor.
	*/
	PoolId DescriptorSetLayout::deriveDescriptorSet(const DescriptorSetCreateParams& rParams_)
	{
		rParams_.validate();
		DescriptorsManager& rDescriptorsManager = getDescriptorsManager();
		PoolId setId = rDescriptorsManager.descriptorSet.add(rParams_);
		DescriptorSet& rSet = rDescriptorsManager.descriptorSet[setId];

		std::vector<PoolId> deadSlotDescriptorIds;
		for(uint32_t i = 0; i < this->descriptorLayouts.allocatedSize(); i++)
		{
			PoolId descriptorLayoutId = PoolId(i, this->descriptorLayouts.generation[i]);
			PoolId descriptorId = rSet.descriptors.add(rParams_.layoutId, rParams_.memoryVisability, setId, descriptorLayoutId);
			if(this->descriptorLayouts.alive[i])
			{
				this->descriptorLayouts.getInternal(i).derivedDescriptors.emplace_back(setId);
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

	/* Binding numbers are the ones the user gave (DescriptorLayout::binding), in any order and sparse, Vulkan only needs them to be unique and to match the shader. One descriptor per binding, so descriptorCount is 1. */
	void DescriptorSetLayout::create()
    {
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.reserve(this->descriptorLayouts.size());
		for(uint32_t i = 0; i < this->descriptorLayouts.allocatedSize(); i++)
		{
			if(!this->descriptorLayouts.alive[i])
			{
				continue;
			}

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = this->descriptorLayouts.objects[i].binding;
			binding.descriptorType = this->descriptorLayouts.objects[i].getVkType();
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

	/*
		Adds one pool sized for setsPerPool sets: per (type, dynamic) kind descriptor layouts x setsPerPool x copies of a set. A set has framesAtFlightCount copies when any of its descriptors is per frame, so the pool always assumes that worst case.
		Free-able, so a removed set gives its place back. Without descriptor layouts there is nothing to size (a set of such a layout can't be created anyway).
	*/
	void DescriptorSetLayout::addPool()
	{
		DescriptorPool pool;
		std::array<uint32_t, DescriptorTypesCount * 2> countPerKind = {}; // [type * 2 + dynamic]
		for(uint32_t i = 0; i < this->descriptorLayouts.allocatedSize(); i++)
		{
			if(this->descriptorLayouts.alive[i])
			{
				countPerKind[this->descriptorLayouts.objects[i].type * 2 + (this->descriptorLayouts.objects[i].dynamic ? 1 : 0)]++;
			}
		}

		uint32_t maxCopies = this->setsPerPool * Globals::renderer.framesAtFlightCount;
		for(uint32_t kind = 0; kind < countPerKind.size(); kind++)
		{
			if(countPerKind[kind] > 0)
			{
				pool.setDescriptorCountPerType(static_cast<DescriptorTypeEnum>(kind / 2), kind % 2 == 1, countPerKind[kind] * maxCopies);
			}
		}
		if(pool.poolSizePerType.empty())
		{
			throw std::runtime_error(std::format("Descriptor Set Layout {}: Can't allocate a Descriptor Set, the layout has no descriptor layouts.", this->name));
		}
		pool.create(maxCopies);
		this->pools.emplace_back(pool);
	}

	/*
		A pool of a fixed size can run out, and that is not an error here: vkAllocateDescriptorSets reports VK_ERROR_OUT_OF_POOL_MEMORY (or VK_ERROR_FRAGMENTED_POOL) when the pool has no room, and the spec guarantees a failed allocation leaves the pool unchanged. So every pool is tried in order (a removed set freed its place in its own pool), and only when none has room a new one is added. Any other failure throws.
		The device has no limit on the number of sets, so there is nothing else to check. The per-set descriptor limits are checked when the set's memory is allocated.
	*/
	uint32_t DescriptorSetLayout::allocateSets(uint32_t count_, VkDescriptorSet* pHandles_)
	{
		std::vector<VkDescriptorSetLayout> setLayouts(count_, this->vkHandle);
		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorSetCount = count_;
		setAllocInfo.pSetLayouts = setLayouts.data();

		for(uint32_t i = 0; i <= this->pools.size(); i++)
		{
			if(i == this->pools.size())
			{
				this->addPool();
			}
			setAllocInfo.descriptorPool = this->pools[i].vkHandle;
			VkResult result = vkAllocateDescriptorSets(getMainDevice().logicalDevice, &setAllocInfo, pHandles_);
			if(result == VK_SUCCESS)
			{
				return i;
			}
			if(result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL)
			{
				throw std::runtime_error(std::format("Descriptor Set Layout {}: Failed to allocate Descriptor Sets!", this->name));
			}
		}
		throw std::runtime_error(std::format("Descriptor Set Layout {}: A new Descriptor Pool couldn't hold {} Descriptor Sets.", this->name, count_));
	}

	/*
		A PipelineLayout copies the set layout handles when it is created, so every layout that uses this one has to be rebuilt (and with it its pipelines).
		vkDestroyPipeline (and the layout destroys) require that submitted commands using the object have completed, and the pipelines rebuilt below may be recorded in frames still in flight, so the device is idle before anything is destroyed.
	*/
	void DescriptorSetLayout::recreate()
	{
		vkDeviceWaitIdle(getMainDevice().logicalDevice);
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
		for(DescriptorPool& rPool : this->pools)
		{
			rPool.destroy();
		}
		this->pools.clear();
        if(this->vkHandle != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(getMainDevice().logicalDevice, this->vkHandle, nullptr); 
			this->vkHandle = VK_NULL_HANDLE;
        }
    }
}
