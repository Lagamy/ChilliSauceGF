#include "DescriptorSet.h"
#include "Api.h"
#include "Globals.h"
#include "Descriptor.h"
#include "MemoryManager.h"
#include "Utilities.h"
#include <format>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics 
{   
	DescriptorSet::DescriptorSet(PoolId setLayout_, AllocatorTypeEnum allocatorType_) : setLayout(setLayout_), allocatorType(allocatorType_)
	{
		this->descriptorType = getDescriptorsManager().layouts[setLayout_].descriptorType;
	}

	VkDescriptorSet DescriptorSet::get() const
	{
		return this->vkHandle;
	}

	/* Descriptors and blueprints share a slot, so the blueprint id's slot is the descriptor. Only the slot is used: the generations of the two Pools are independent. */
	Descriptor& DescriptorSet::getDescriptor(PoolId descriptorId_)
	{
		#ifdef ENGINE_DEBUG
			if(descriptorId_.id >= this->descriptors.allocatedSize() || !this->descriptors.alive[descriptorId_.id])
			{
				throw std::runtime_error(std::format("Descriptor Set: Descriptor in slot {} doesn't exist.", descriptorId_.id));
			}
			if(!this->descriptorsMemoryAllocated)
			{
				throw std::runtime_error("Descriptor Set: Can't set descriptor values before allocateDescriptorsMemory.");
			}
		#endif
		return this->descriptors.getInternal(descriptorId_.id);
	}

	ReflectionLayout& DescriptorSet::getReflectionLayout(Descriptor& rDescriptor_)
	{
		DescriptorBlueprint& rBlueprint = rDescriptor_.getBlueprint();
		return getReflectionSystem().layoutsPerBufferType[rBlueprint.bufferType - 1][rBlueprint.layoutId]; // - 1 due to INDEX layout not existing
	}

	StagingDataContainer DescriptorSet::getStagingContainer(const Descriptor& rDescriptor_, uint64_t inEntryOffset_, uint64_t size_)
	{
		return getMemoryManager().getStagingContainer(rDescriptor_.memoryEntryId, inEntryOffset_, size_);
	}

	SharedDataContainer DescriptorSet::getSharedContainer(const Descriptor& rDescriptor_)
	{
		return getMemoryManager().getSharedContainer(rDescriptor_.memoryEntryId);
	}

	/*
		Allocates one MemoryEntry per descriptor, sized from its blueprint's ReflectionLayout (one repeat unit). Nothing is uploaded: a GPU_LOCAL entry's bytes are undefined until the first setDescriptorMemberValue was copied.
		Static entries have to be added before StaticAllocator::allocateAndUpload, which is why this is separate from create.
	*/
	void DescriptorSet::allocateDescriptorsMemory()
	{
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(!this->descriptors.alive[i])
			{
				continue;
			}

			Descriptor& rDescriptor = this->descriptors.getInternal(i);
			DescriptorBlueprint& rBlueprint = rDescriptor.getBlueprint();
			if(getReflectionLayout(rDescriptor).size == 0)
			{
				throw std::runtime_error(std::format("Descriptor Set: Blueprint {} has no members, a buffer can't have size 0.", rBlueprint.name));
			}

			rDescriptor.memoryEntryId = allocateMemoryEntry(rBlueprint.name.c_str(), this->allocatorType, rDescriptor.memoryVisability, rBlueprint.bufferType, rBlueprint.layoutId, 1);
			getMemoryManager().getEntry(rDescriptor.memoryEntryId).descriptorSet = rDescriptor.setId;
		}
		this->descriptorsMemoryAllocated = true;
	}

	/*
		Creates the pool (one pool per set, sized for exactly its descriptors), allocates the set from the layout and points every binding at its entry's buffer range in a single vkUpdateDescriptorSets.
		The set is fresh and never bound yet, so no pending command buffer uses it (a set may only be updated while none does, since there is no update-after-bind).
		VkDescriptorBufferInfo::offset has to respect minUniformBufferOffsetAlignment / minStorageBufferOffsetAlignment: dynamic entries are their own buffer (offset 0), static ones sit at inBufferFirstByte in the shared heap buffer.
	*/
    void DescriptorSet::create()
    {
		#ifdef ENGINE_DEBUG
			if(!this->descriptorsMemoryAllocated)
			{
				throw std::runtime_error("Descriptor Set: Can't create before allocateDescriptorsMemory.");
			}
		#endif

		uint32_t descriptorsCount = 0;
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(this->descriptors.alive[i])
			{
				descriptorsCount++;
			}
		}
		if(descriptorsCount == 0)
		{
			throw std::runtime_error("Descriptor Set: Can't create a set without descriptors.");
		}

		this->pool.setDescriptorCountPerType(this->descriptorType, descriptorsCount);
		this->pool.create(1);

		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = this->pool.vkHandle;
		setAllocInfo.descriptorSetCount = 1;
		setAllocInfo.pSetLayouts = &getDescriptorsManager().layouts[this->setLayout].vkHandle;

		VkResult result = vkAllocateDescriptorSets(getMainDevice().logicalDevice, &setAllocInfo, &this->vkHandle);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Descriptor Sets!");
		}

		/* The writes keep pointers into bufferInfos, so it is reserved up front and never reallocates. */
		MemoryManager& rMemoryManager = getMemoryManager(); 
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(descriptorsCount);
		std::vector<VkWriteDescriptorSet> updateSetWrites; 
		updateSetWrites.reserve(descriptorsCount);
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(!this->descriptors.alive[i])
			{
				continue;
			}

			Descriptor& rDescriptor = this->descriptors.getInternal(i);
			MemoryEntry& rMemoryEntry = rMemoryManager.getEntry(rDescriptor.memoryEntryId); 
			Buffer& rBuffer = rMemoryManager.getBuffer(rMemoryEntry);
			if(rBuffer.get() == VK_NULL_HANDLE)
			{
				throw std::runtime_error(std::format("Descriptor Set: Buffer of {} doesn't exist yet. Static entries get it in StaticAllocator::allocateAndUpload.", rMemoryEntry.name));
			}

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = rBuffer.get();
			bufferInfo.offset = rMemoryEntry.isForDynamic ? 0 : rMemoryEntry.inBufferFirstByte;
			bufferInfo.range = rMemoryEntry.size;
			bufferInfos.emplace_back(bufferInfo);

			VkWriteDescriptorSet updateSetWrite = {};
			updateSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			updateSetWrite.dstSet = this->vkHandle;
			updateSetWrite.dstBinding = i;
			updateSetWrite.dstArrayElement = 0;
			updateSetWrite.descriptorType = descriptorTypes[this->descriptorType];
			updateSetWrite.descriptorCount = 1;
			updateSetWrite.pBufferInfo = &bufferInfos.back();
			updateSetWrites.emplace_back(updateSetWrite);

			rDescriptor.created = true;
		}
		vkUpdateDescriptorSets(getMainDevice().logicalDevice, updateSetWrites.size(), updateSetWrites.data(), 0, nullptr);
    }

	/* Destroying the pool frees the set allocated from it. The memory entries stay owned by the MemoryManager and are freed with it. */
	void DescriptorSet::destroy()
	{
		this->pool.destroy();
		this->vkHandle = VK_NULL_HANDLE;
		this->framesInFlightMask = 0;
	}

	void DescriptorSet::markUsedInCurrentFrame()
	{
		#ifdef ENGINE_DEBUG
			if(getCurrentFrameIndex() >= 32)
			{
				throw std::runtime_error("Descriptor Set: framesInFlightMask has 32 bits, can't track more frames in flight.");
			}
		#endif
		this->framesInFlightMask |= 1u << getCurrentFrameIndex();
	}

	/*
		A CPU_SHARED descriptor is read by the GPU while any frame whose command buffer binds the set is still executing. The host may overwrite it only after those reads finished:
		a write-after-read hazard needs an execution dependency and no memory barrier, and a fence that vkWaitForFences saw signaled is that dependency (the memory is coherent, so no flush either).
		Every FRAME pass signals its own fence for the frame (PassesGraph::waitForFrame waits on all of them), so waiting on the marked frames covers every read. Waiting doesn't reset the fences, the submit does.
		Only FRAME passes are covered: a ONESHOT command buffer has no frame fence. Unhandled, has to be fixed before compute (ONESHOT passes that bind sets) is implemented.
	*/
	void DescriptorSet::waitForFramesUsingSet()
	{
		for(uint32_t frame = 0; this->framesInFlightMask != 0; frame++)
		{
			if(this->framesInFlightMask & (1u << frame))
			{
				Globals::renderer.passesGraph.waitForFrame(frame);
				this->framesInFlightMask &= ~(1u << frame);
			}
		}
	}
}
