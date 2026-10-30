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

	/* The copy of the current frame if the set has one per frame in flight (see create), otherwise the only one. */
	VkDescriptorSet DescriptorSet::get() const
	{
		#ifdef ENGINE_DEBUG
			if(this->vkHandles.empty())
			{
				throw std::runtime_error("Descriptor Set: Can't get before create.");
			}
		#endif
		return this->vkHandles[this->vkHandles.size() == 1 ? 0 : getCurrentFrameIndex()];
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

	StagingDataContainer DescriptorSet::getStagingContainer(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_)
	{
		return getMemoryManager().getStagingContainer(entryId_, inEntryOffset_, size_);
	}

	SharedDataContainer DescriptorSet::getSharedContainer(MemoryEntryId entryId_)
	{
		return getMemoryManager().getSharedContainer(entryId_);
	}

	/*
		Marks a descriptor's entry as readOnly (see MemoryEntry::readOnly). The flag is read by allocateDescriptorsMemory when the entry is created, so it can't change afterwards.
		descriptorId_ is the blueprint id, as in setDescriptorMemberValue.
	*/
	void DescriptorSet::setDescriptorReadOnly(PoolId descriptorId_, bool readOnly_)
	{
		if(this->descriptorsMemoryAllocated)
		{
			throw std::runtime_error("Descriptor Set: Can't change readOnly after allocateDescriptorsMemory.");
		}
		if(descriptorId_.id >= this->descriptors.allocatedSize() || !this->descriptors.alive[descriptorId_.id])
		{
			throw std::runtime_error(std::format("Descriptor Set: Descriptor in slot {} doesn't exist.", descriptorId_.id));
		}
		this->descriptors.getInternal(descriptorId_.id).readOnly = readOnly_;
	}

	/*
		Allocates one MemoryEntry per descriptor (one per frame in flight and a zeroed shadow for a per-frame descriptor, see Descriptor::isPerFrame), sized from its blueprint's ReflectionLayout (one repeat unit). Nothing is uploaded: a GPU_LOCAL entry's bytes are undefined until the first setDescriptorMemberValue was copied.
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

			uint32_t copiesCount = rDescriptor.isPerFrame() ? Globals::renderer.framesAtFlightCount : 1;
			rDescriptor.memoryEntryIds.reserve(copiesCount);
			for(uint32_t copy = 0; copy < copiesCount; copy++)
			{
				MemoryEntryId entryId = allocateMemoryEntry(rBlueprint.name.c_str(), this->allocatorType, rDescriptor.memoryVisability, rBlueprint.bufferType, rBlueprint.layoutId, 1, rDescriptor.readOnly);
				getMemoryManager().getEntry(entryId).descriptorSet = rDescriptor.setId;
				rDescriptor.memoryEntryIds.emplace_back(entryId);
			}
			if(rDescriptor.isPerFrame())
			{
				rDescriptor.shadow.data.assign(getReflectionLayout(rDescriptor).size, 0);
				rDescriptor.copyVersions.assign(copiesCount, 0);
			}
		}
		this->descriptorsMemoryAllocated = true;
	}

	/*
		Creates the pool (one pool per set, sized for exactly its descriptors times the copies), allocates the copies of the set (one per frame in flight if any descriptor isPerFrame, otherwise one) from the layout and points every binding at its entry's buffer range in a single vkUpdateDescriptorSets.
		The sets are fresh and never bound yet, so no pending command buffer uses it (a set may only be updated while none does, since there is no update-after-bind).
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

		bool hasPerFrameDescriptor = false;
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(this->descriptors.alive[i] && this->descriptors.getInternal(i).isPerFrame())
			{
				hasPerFrameDescriptor = true;
			}
		}
		uint32_t copiesCount = hasPerFrameDescriptor ? Globals::renderer.framesAtFlightCount : 1;

		this->pool.setDescriptorCountPerType(this->descriptorType, descriptorsCount * copiesCount);
		this->pool.create(copiesCount);

		std::vector<VkDescriptorSetLayout> setLayouts(copiesCount, getDescriptorsManager().layouts[this->setLayout].vkHandle);
		this->vkHandles.resize(copiesCount);

		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = this->pool.vkHandle;
		setAllocInfo.descriptorSetCount = copiesCount;
		setAllocInfo.pSetLayouts = setLayouts.data();

		VkResult result = vkAllocateDescriptorSets(getMainDevice().logicalDevice, &setAllocInfo, this->vkHandles.data());
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Descriptor Sets!");
		}

		/* The writes keep pointers into bufferInfos, so it is reserved up front and never reallocates. Copy f of the set points every binding at getEntryId(f): a per-frame descriptor gets its own entry per copy, the others share one. */
		MemoryManager& rMemoryManager = getMemoryManager(); 
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(descriptorsCount * copiesCount);
		std::vector<VkWriteDescriptorSet> updateSetWrites; 
		updateSetWrites.reserve(descriptorsCount * copiesCount);
		for(uint32_t copy = 0; copy < copiesCount; copy++)
		{
			for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
			{
				if(!this->descriptors.alive[i])
				{
					continue;
				}

				Descriptor& rDescriptor = this->descriptors.getInternal(i);
				MemoryEntry& rMemoryEntry = rMemoryManager.getEntry(rDescriptor.getEntryId(copy)); 
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
				updateSetWrite.dstSet = this->vkHandles[copy];
				updateSetWrite.dstBinding = i;
				updateSetWrite.dstArrayElement = 0;
				updateSetWrite.descriptorType = descriptorTypes[this->descriptorType];
				updateSetWrite.descriptorCount = 1;
				updateSetWrite.pBufferInfo = &bufferInfos.back();
				updateSetWrites.emplace_back(updateSetWrite);

				rDescriptor.created = true;
			}
		}
		vkUpdateDescriptorSets(getMainDevice().logicalDevice, updateSetWrites.size(), updateSetWrites.data(), 0, nullptr);
    }

	/* Destroying the pool frees the set allocated from it. The memory entries stay owned by the MemoryManager and are freed with it. */
	void DescriptorSet::destroy()
	{
		this->pool.destroy();
		this->vkHandles.clear();
	}

	/*
		CPU-side rule only, no Vulkan sync: a readOnly entry is written without waiting, which is only safe while no GPU read can overlap the host write. A command buffer can only read the descriptor once it is submitted, so DescriptorsManager::lockBoundDescriptors calls this right after the submit that recorded a draw reading it and later writes throw (MemoryManager::lockEntry).
		Locks every copy of the descriptor, once. Descriptors no draw read stay writable.
	*/
	void DescriptorSet::lockReadOnlyDescriptor(PoolId descriptorId_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		if(!rDescriptor.readOnly || rDescriptor.readOnlyLocked)
		{
			return;
		}

		for(const MemoryEntryId& entryId : rDescriptor.memoryEntryIds)
		{
			getMemoryManager().lockEntry(entryId);
		}
		rDescriptor.readOnlyLocked = true;
	}

	/*
		Vulkan rule: a pipeline barrier orders commands only within one queue (spec: Synchronization and Cache Control -> Pipeline Barriers), a semaphore wait orders across queues (spec: Semaphores). Device prefers the graphics family for TRANSFER and COMPUTE, and takes queue 0 of each family, so normally the copy batch and the draws are on one VkQueue: the batch's final TRANSFER_WRITE -> read barrier orders every update copy before the draws submitted after it, and the start barrier orders it after earlier draws. With separate families QueueTransferSync's done semaphores do the same between the queues.
		The barriers can't help a draw that is recorded before the copy exists: a GPU_LOCAL descriptor whose first write is still pending has no data yet (an allocated entry is undefined bytes), so the CPU skips the draw until the first copy was observed finished.
		Only the first data is checked (readOnly or not): once a check saw it settled, firstDataSettled stays true and later updates are covered by the barriers. A descriptor nothing was written to has nothing to wait for.
	*/
	bool DescriptorSet::isDescriptorSettled(PoolId descriptorId_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		if(rDescriptor.memoryVisability != GPU_LOCAL || rDescriptor.firstDataSettled || !rDescriptor.gpuWritten)
		{
			return true;
		}

		for(const MemoryEntryId& entryId : rDescriptor.memoryEntryIds)
		{
			if(getMemoryManager().hasPendingWrite(entryId))
			{
				return false;
			}
		}
		rDescriptor.firstDataSettled = true;
		return true;
	}

	/*
		Vulkan rule: frame_'s previous submission read this frame's entries. vkWaitForFences returning signaled (the caller waited on frame_'s fences, PassesGraph::waitForFrame) makes that read happen-before the host, and a write-after-read hazard needs only that execution dependency. The memory is HOST_COHERENT, so no flush.
		Writes by the host before vkQueueSubmit are visible to that submission (host write ordering guarantee), and the copy happens before this frame's submit, so no barrier is needed either. Only the entry of frame_ is touched: the other frames' entries may still be read by the GPU.
		Whole-entry copy of every shadow whose version is newer than the one frame_'s entry holds. Sets that are not created yet have no buffers to write into and are skipped.
	*/
	void DescriptorSet::syncToFrame(uint32_t frame_)
	{
		if(this->vkHandles.empty())
		{
			return;
		}

		MemoryManager& rMemoryManager = getMemoryManager();
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(!this->descriptors.alive[i])
			{
				continue;
			}

			Descriptor& rDescriptor = this->descriptors.getInternal(i);
			if(!rDescriptor.isPerFrame() || rDescriptor.copyVersions[frame_] == rDescriptor.version)
			{
				continue;
			}

			rMemoryManager.writeShared(rMemoryManager.getEntry(rDescriptor.getEntryId(frame_)), 0, rDescriptor.shadow.data.data(), rDescriptor.shadow.data.size());
			rDescriptor.copyVersions[frame_] = rDescriptor.version;
		}
	}
}
