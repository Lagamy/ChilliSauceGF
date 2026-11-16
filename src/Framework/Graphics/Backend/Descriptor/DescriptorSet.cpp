#include "DescriptorSet.h"
#include "Api.h"
#include "Globals.h"
#include "Descriptor.h"
#include "MemoryManager.h"
#include "Utilities.h"
#include <array>
#include <format>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics 
{   
	DescriptorSet::DescriptorSet(PoolId setLayout_, AllocatorTypeEnum allocatorType_) : setLayout(setLayout_), allocatorType(allocatorType_)
	{
	}

	/* The alignment of a dynamic offset (and of the entry start) depends on the buffer kind of the binding. */
	static uint64_t getDynamicAlignment(DescriptorTypeEnum type_)
	{
		const VkPhysicalDeviceLimits& rLimits = getMainDevice().properties.limits;
		return type_ == UNIFORM_DESCRIPTOR ? rLimits.minUniformBufferOffsetAlignment : rLimits.minStorageBufferOffsetAlignment;
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

	/* Descriptors and descriptor layouts share a slot, so the descriptor layout id's slot is the descriptor. Only the slot is used: the generations of the two Pools are independent. */
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

	const VariableBlueprint& DescriptorSet::getVariable(PoolId descriptorId_, PoolId variableId_)
	{
		DescriptorLayout& rDescriptorLayout = this->getDescriptor(descriptorId_).getDescriptorLayout();
		const VariableRef& rVariableRef = rDescriptorLayout.getVariable(variableId_);
		return rDescriptorLayout.getMember(rVariableRef.memberId).layout.variableBlueprints[rVariableRef.variableId];
	}

	/*
		A variable directly in the block has no repeat layer of its own, a variable of a struct member has one per layer of the struct. The repeatUnitId_ overloads of the setters are the one-layer case, so for a variable without layers it has to be 0 and no id is passed on.
	*/
	std::vector<uint32_t> DescriptorSet::makeSingleLayerIds(PoolId descriptorId_, PoolId variableId_, uint32_t repeatUnitId_)
	{
		DescriptorLayout& rDescriptorLayout = this->getDescriptor(descriptorId_).getDescriptorLayout();
		const MemberBlueprint& rMember = rDescriptorLayout.getMember(rDescriptorLayout.getVariable(variableId_).memberId);
		if(rMember.kind == VARIABLE_MEMBER)
		{
			if(repeatUnitId_ != 0)
			{
				throw std::runtime_error(std::format("Descriptor Set: {} is a variable directly in the block, it has no repeat unit {}.", rMember.name, repeatUnitId_));
			}
			return {};
		}
		return {repeatUnitId_};
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
		descriptorId_ is the descriptor layout id, as in setDescriptorMemberValue.
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
		A plain uniform binding sees the whole entry and a dynamic one a single element of the outermost layer, both have to fit maxUniformBufferRange (often 64KB), which storage buffers don't have.
	*/
	void DescriptorSet::checkDynamic(Descriptor& rDescriptor_, bool dynamic_)
	{
		bool isDynamic = rDescriptor_.isDynamic();
		if(isDynamic && !dynamic_)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} has a dynamic layer, write it with the setDynamicDescriptorMember... functions.", rDescriptor_.getDescriptorLayout().name));
		}
		if(!isDynamic && dynamic_)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} has no dynamic layer, setDynamicDescriptorMember... is only for dynamic descriptor layouts.", rDescriptor_.getDescriptorLayout().name));
		}
	}

	std::vector<uint32_t> DescriptorSet::makeDynamicIds(uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIds_)
	{
		std::vector<uint32_t> ids = {dynamicUnitId_};
		ids.insert(ids.end(), innerIds_.begin(), innerIds_.end());
		return ids;
	}

	std::vector<uint32_t> DescriptorSet::getDynamicOffsets(std::span<const uint32_t> unitIds_)
	{
		std::vector<uint32_t> offsets;
		if(!getDescriptorsManager().layouts[this->setLayout].dynamic)
		{
			if(!unitIds_.empty())
			{
				throw std::runtime_error("Descriptor Set: Dynamic unit ids were given for a set whose layout has no dynamic descriptor layout.");
			}
			return offsets;
		}
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(!this->descriptors.alive[i])
			{
				continue;
			}
			Descriptor& rDescriptor = this->descriptors.getInternal(i);
			if(!rDescriptor.isDynamic())
			{
				continue;
			}
			uint32_t unitId = offsets.size() < unitIds_.size() ? unitIds_[offsets.size()] : 0;
			if(unitId >= rDescriptor.shape.dynamicLayer.count)
			{
				throw std::runtime_error(std::format("Descriptor Set: Dynamic unit id {} of {} is out of range, dynamicCount is {}.", unitId, rDescriptor.getDescriptorLayout().name, rDescriptor.shape.dynamicLayer.count));
			}
			offsets.push_back(static_cast<uint32_t>(unitId * rDescriptor.shape.dynamicLayer.stride));
		}
		if(!unitIds_.empty() && unitIds_.size() != offsets.size()) // also catches ids given for a set without dynamic descriptors
		{
			throw std::runtime_error(std::format("Descriptor Set: {} dynamic unit ids given, the set has {} dynamic descriptors.", unitIds_.size(), offsets.size()));
		}
		return offsets;
	}

	void DescriptorSet::validateShape(Descriptor& rDescriptor_)
	{
		uint64_t seenBytes = 0;
		if(rDescriptor_.getType() != UNIFORM_DESCRIPTOR)
		{
			return;
		}
		seenBytes = rDescriptor_.isDynamic() ? rDescriptor_.shape.blockSize : rDescriptor_.shape.getSize();

		uint64_t maxRange = getMainDevice().properties.limits.maxUniformBufferRange;
		if(seenBytes > maxRange)
		{
			throw std::runtime_error(std::format("Descriptor Set: Descriptor Layout {} binds {} bytes as a uniform, the device allows {}. Use a storage buffer or a dynamic uniform.", rDescriptor_.getDescriptorLayout().name, seenBytes, maxRange));
		}
	}

	/*
		Allocates one MemoryEntry per descriptor (one per frame in flight and a zeroed shadow for a per-frame descriptor, see Descriptor::isPerFrame), sized from its descriptor layout's block (DescriptorLayout::makeShape). Nothing is uploaded: a GPU_LOCAL entry's bytes are undefined until the first setDescriptorMemberValue was copied.
		Static entries have to be added before StaticAllocator::allocateAndUpload, which is why this is separate from create.
	*/
	void DescriptorSet::allocateDescriptorsMemory()
	{
		uint32_t uniformCount = 0;
		uint32_t storageCount = 0;
		uint32_t dynamicUniformCount = 0;
		uint32_t dynamicStorageCount = 0;
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(!this->descriptors.alive[i])
			{
				continue;
			}

			Descriptor& rDescriptor = this->descriptors.getInternal(i);
			DescriptorLayout& rDescriptorLayout = rDescriptor.getDescriptorLayout();
			/* Every dynamic offset has to be a multiple of minUniformBufferOffsetAlignment (uniform) / minStorageBufferOffsetAlignment (storage), so the units are padded to it. */
			rDescriptor.shape = rDescriptorLayout.makeShape(getDynamicAlignment(rDescriptor.getType()));
			this->validateShape(rDescriptor);
			(rDescriptor.getType() == UNIFORM_DESCRIPTOR ? uniformCount : storageCount)++;
			if(rDescriptor.isDynamic())
			{
				(rDescriptor.getType() == UNIFORM_DESCRIPTOR ? dynamicUniformCount : dynamicStorageCount)++;
			}

			uint32_t copiesCount = rDescriptor.isPerFrame() ? Globals::renderer.framesAtFlightCount : 1;
			rDescriptor.memoryEntryIds.reserve(copiesCount);
			for(uint32_t copy = 0; copy < copiesCount; copy++)
			{
				MemoryEntryId entryId = allocateMemoryEntryRaw(rDescriptorLayout.name.c_str(), this->allocatorType, rDescriptor.memoryVisability, rDescriptorLayout.bufferType, rDescriptor.shape.getSize(), rDescriptor.readOnly);
				getMemoryManager().getEntry(entryId).descriptorSet = rDescriptor.setId;
				rDescriptor.memoryEntryIds.emplace_back(entryId);
			}
			if(rDescriptor.isPerFrame())
			{
				rDescriptor.shadow.data.assign(rDescriptor.shape.getSize(), 0);
				rDescriptor.copyVersions.assign(copiesCount, 0);
			}
		}
		/* maxDescriptorSetUniformBuffers / maxDescriptorSetStorageBuffers count the dynamic descriptors of that type too, so the totals are checked next to the dynamic-only limits. */
		const VkPhysicalDeviceLimits& rLimits = getMainDevice().properties.limits;
		if(uniformCount > rLimits.maxDescriptorSetUniformBuffers)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} uniform descriptors in one set, the device allows {}.", uniformCount, rLimits.maxDescriptorSetUniformBuffers));
		}
		if(storageCount > rLimits.maxDescriptorSetStorageBuffers)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} storage descriptors in one set, the device allows {}.", storageCount, rLimits.maxDescriptorSetStorageBuffers));
		}
		if(dynamicUniformCount > rLimits.maxDescriptorSetUniformBuffersDynamic)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} dynamic uniform descriptors in one set, the device allows {}.", dynamicUniformCount, rLimits.maxDescriptorSetUniformBuffersDynamic));
		}
		if(dynamicStorageCount > rLimits.maxDescriptorSetStorageBuffersDynamic)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} dynamic storage descriptors in one set, the device allows {}.", dynamicStorageCount, rLimits.maxDescriptorSetStorageBuffersDynamic));
		}
		this->descriptorsMemoryAllocated = true;
	}

	/*
		Allocates the copies of the set (one per frame in flight if any descriptor isPerFrame, otherwise one) from the layout's pools (DescriptorSetLayout::allocateSets picks one with room) and points every binding at its entry's buffer range in a single vkUpdateDescriptorSets.
		The sets are fresh and never bound yet, so no pending command buffer uses it (a set may only be updated while none does, since there is no update-after-bind).
		VkDescriptorBufferInfo::offset has to respect minUniformBufferOffsetAlignment / minStorageBufferOffsetAlignment: dynamic entries are their own buffer (offset 0), static ones sit at inBufferFirstByte in the shared heap buffer, which StaticAllocator::addEntry aligns for uniform and storage entries.
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

		DescriptorSetLayout& rLayout = getDescriptorsManager().layouts[this->setLayout];
		this->vkHandles.resize(copiesCount);
		this->poolId = rLayout.allocateSets(copiesCount, this->vkHandles.data());

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
				if(rDescriptor.isDynamic() && bufferInfo.offset % getDynamicAlignment(rDescriptor.getType()) != 0)
				{
					/* The dynamic offsets are multiples of the alignment (padded stride), so the sum only is if the entry starts aligned too. */
					throw std::runtime_error(std::format("Descriptor Set: Dynamic entry {} starts at byte {}, not a multiple of the offset alignment of its buffer type.", rMemoryEntry.name, bufferInfo.offset));
				}
				/* A dynamic binding sees one block: the dynamic offset of the bind moves this window inside the entry, which holds all of them. The others see the whole entry. */
				bufferInfo.range = rDescriptor.isDynamic() ? rDescriptor.shape.blockSize : rMemoryEntry.size;
				bufferInfos.emplace_back(bufferInfo);

				VkWriteDescriptorSet updateSetWrite = {};
				updateSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				updateSetWrite.dstSet = this->vkHandles[copy];
				updateSetWrite.dstBinding = i;
				updateSetWrite.dstArrayElement = 0;
				updateSetWrite.descriptorType = rDescriptor.getDescriptorLayout().getVkType();
				updateSetWrite.descriptorCount = 1;
				updateSetWrite.pBufferInfo = &bufferInfos.back();
				updateSetWrites.emplace_back(updateSetWrite);

				rDescriptor.created = true;
			}
		}
		vkUpdateDescriptorSets(getMainDevice().logicalDevice, updateSetWrites.size(), updateSetWrites.data(), 0, nullptr);
    }

	/*
		Gives the VkDescriptorSets back to its pool in the layout (the caller guarantees the GPU is done with them). The memory entries stay owned by the MemoryManager.
		If the layout's pools are already gone (shutdown, or the layout was recreated) the sets went with it.
	*/
	void DescriptorSet::destroy()
	{
		DescriptorSetLayout& rLayout = getDescriptorsManager().layouts[this->setLayout];
		if(this->poolId < rLayout.pools.size() && !this->vkHandles.empty())
		{
			vkFreeDescriptorSets(getMainDevice().logicalDevice, rLayout.pools[this->poolId].vkHandle, this->vkHandles.size(), this->vkHandles.data());
		}
		this->vkHandles.clear();
		this->poolId = UninitializedId;
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
