#include "DescriptorSet.h"
#include "Api.h"
#include "Globals.h"
#include "Descriptor.h"
#include "MemoryManager.h"
#include "DescriptorLayoutUpdate.h"
#include "Utilities.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics 
{   
	DescriptorSet::DescriptorSet(const DescriptorSetCreateParams& rParams_) : setLayout(rParams_.layoutId), allocatorType(rParams_.allocatorType), params(rParams_)
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

		/* Vulkan orders the dynamic offsets of a bind by binding number, which is not the slot order now that bindings are chosen by the user. */
		std::vector<Descriptor*> dynamicDescriptors;
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(this->descriptors.alive[i] && this->descriptors.getInternal(i).isDynamic())
			{
				dynamicDescriptors.push_back(&this->descriptors.getInternal(i));
			}
		}
		std::sort(dynamicDescriptors.begin(), dynamicDescriptors.end(), [](Descriptor* pLeft_, Descriptor* pRight_)
		{
			return pLeft_->getDescriptorLayout().binding < pRight_->getDescriptorLayout().binding;
		});

		for(Descriptor* pDescriptor : dynamicDescriptors)
		{
			Descriptor& rDescriptor = *pDescriptor;
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

	/* A per frame descriptor's entry has one copy per frame in flight, kept by the MemoryManager (shadow, versions, sync). The shape has to be built already (allocateDescriptorsMemory does it once, a load reuses it). */
	void DescriptorSet::allocateDescriptorMemory(Descriptor& rDescriptor_)
	{
		DescriptorLayout& rDescriptorLayout = rDescriptor_.getDescriptorLayout();
		MemoryEntryCreateParams entryParams = {};
		entryParams.name = rDescriptorLayout.name.c_str();
		entryParams.allocatorType = this->allocatorType;
		entryParams.memoryVisability = rDescriptor_.memoryVisability;
		entryParams.bufferType = rDescriptorLayout.bufferType;
		entryParams.size = rDescriptor_.shape.getSize();
		entryParams.access.accessMode = rDescriptor_.readOnly ? MemoryAccessModeEnum::READ_ONLY : MemoryAccessModeEnum::READ_AND_WRITE;
		if(rDescriptor_.isPerFrame())
		{
			entryParams.access.copyCountMode = CopyCountModeEnum::FOLLOW_FRAMES_IN_FLIGHT;
		}
		rDescriptor_.memoryEntryId = allocateMemoryEntryRaw(entryParams);
		MemoryEntry& rEntry = getMemoryManager().getEntry(rDescriptor_.memoryEntryId);
		rEntry.descriptorSet = rDescriptor_.setId;
		for(const MemoryEntryId& copyId : rEntry.copyEntryIds)
		{
			getMemoryManager().getEntry(copyId).descriptorSet = rDescriptor_.setId;
		}
	}

	/*
		Builds the shape of one descriptor from its descriptor layout, allocates its entry (see allocateDescriptorMemory) and sizes its backup.
		Every dynamic offset has to be a multiple of minUniformBufferOffsetAlignment (uniform) / minStorageBufferOffsetAlignment (storage), so the units are padded to it.
	*/
	void DescriptorSet::setupDescriptorMemory(Descriptor& rDescriptor_)
	{
		rDescriptor_.shape = rDescriptor_.getDescriptorLayout().makeShape(getDynamicAlignment(rDescriptor_.getType()));
		this->validateShape(rDescriptor_);
		this->allocateDescriptorMemory(rDescriptor_);
		rDescriptor_.residency.backup.data.assign(this->mirrorsWrites(rDescriptor_) ? rDescriptor_.shape.getSize() : 0, 0);
		rDescriptor_.residency.lastUsedTime = std::chrono::steady_clock::now();
	}

	void DescriptorSet::checkDynamicLimits()
	{
		uint32_t dynamicUniformCount = 0;
		uint32_t dynamicStorageCount = 0;
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(this->descriptors.alive[i] && this->descriptors.getInternal(i).isDynamic())
			{
				(this->descriptors.getInternal(i).getType() == UNIFORM_DESCRIPTOR ? dynamicUniformCount : dynamicStorageCount)++;
			}
		}
		const VkPhysicalDeviceLimits& rLimits = getMainDevice().properties.limits;
		if(dynamicUniformCount > rLimits.maxDescriptorSetUniformBuffersDynamic)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} dynamic uniform descriptors in one set, the device allows {}.", dynamicUniformCount, rLimits.maxDescriptorSetUniformBuffersDynamic));
		}
		if(dynamicStorageCount > rLimits.maxDescriptorSetStorageBuffersDynamic)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} dynamic storage descriptors in one set, the device allows {}.", dynamicStorageCount, rLimits.maxDescriptorSetStorageBuffersDynamic));
		}
	}

	/*
		Allocates one MemoryEntry per descriptor (with one copy per frame in flight for a per-frame descriptor, see Descriptor::isPerFrame), sized from its descriptor layout's block (DescriptorLayout::makeShape). Nothing is uploaded: a GPU_LOCAL entry's bytes are undefined until the first setDescriptorMemberValue was copied.
		Static entries have to be added before StaticAllocator::allocateAndUpload, which is why this is separate from create.
	*/
	void DescriptorSet::allocateDescriptorsMemory()
	{
		for(uint32_t i = 0; i < this->descriptors.allocatedSize(); i++)
		{
			if(this->descriptors.alive[i])
			{
				this->setupDescriptorMemory(this->descriptors.getInternal(i));
			}
		}
		this->checkDynamicLimits();
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

		/* The writes keep pointers into bufferInfos, so it is reserved up front and never reallocates. Copy f of the set points every binding at copy f of the descriptor's entry (makeBindingWrite): a per-frame descriptor has one per frame in flight, the others only one. */
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
				if(!rDescriptor.residency.loaded)
				{
					continue;
				}
				bufferInfos.emplace_back();
				updateSetWrites.emplace_back(this->makeBindingWrite(rDescriptor, copy, bufferInfos.back()));
				rDescriptor.created = true;
			}
		}
		if(!updateSetWrites.empty())
		{
			vkUpdateDescriptorSets(getMainDevice().logicalDevice, updateSetWrites.size(), updateSetWrites.data(), 0, nullptr);
		}
    }

	/* The write that points binding of the descriptor in set copy copy_ at copy copy_ of its entry (MemoryManager::getCopy). */
	VkWriteDescriptorSet DescriptorSet::makeBindingWrite(Descriptor& rDescriptor_, uint32_t copy_, VkDescriptorBufferInfo& rBufferInfo_)
	{
		MemoryManager& rMemoryManager = getMemoryManager();
		const MemoryEntry& rMemoryEntry = rMemoryManager.getCopy(rMemoryManager.getEntry(rDescriptor_.memoryEntryId), copy_);
		Buffer& rBuffer = rMemoryManager.getBuffer(rMemoryEntry);
		if(rBuffer.get() == VK_NULL_HANDLE)
		{
			throw std::runtime_error(std::format("Descriptor Set: Buffer of {} doesn't exist yet. Static entries get it in StaticAllocator::allocateAndUpload.", rMemoryEntry.name));
		}

		rBufferInfo_ = {};
		rBufferInfo_.buffer = rBuffer.get();
		rBufferInfo_.offset = rMemoryEntry.isForDynamic ? 0 : rMemoryEntry.inBufferFirstByte;
		if(rDescriptor_.isDynamic() && rBufferInfo_.offset % getDynamicAlignment(rDescriptor_.getType()) != 0)
		{
			/* The dynamic offsets are multiples of the alignment (padded stride), so the sum only is if the entry starts aligned too. */
			throw std::runtime_error(std::format("Descriptor Set: Dynamic entry {} starts at byte {}, not a multiple of the offset alignment of its buffer type.", rMemoryEntry.name, rBufferInfo_.offset));
		}
		/* A dynamic binding sees one block: the dynamic offset of the bind moves this window inside the entry, which holds all of them. The others see the whole entry. */
		rBufferInfo_.range = rDescriptor_.isDynamic() ? rDescriptor_.shape.blockSize : rMemoryEntry.size;

		VkWriteDescriptorSet updateSetWrite = {};
		updateSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		updateSetWrite.dstSet = this->vkHandles[copy_];
		updateSetWrite.dstBinding = rDescriptor_.getDescriptorLayout().binding;
		updateSetWrite.dstArrayElement = 0;
		updateSetWrite.descriptorType = rDescriptor_.getDescriptorLayout().getVkType();
		updateSetWrite.descriptorCount = 1;
		updateSetWrite.pBufferInfo = &rBufferInfo_;
		return updateSetWrite;
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
		Locks the descriptor's entry, once. Descriptors no draw read stay writable.
	*/
	void DescriptorSet::lockReadOnlyDescriptor(PoolId descriptorId_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		if(!rDescriptor.readOnly || rDescriptor.readOnlyLocked || !rDescriptor.residency.loaded)
		{
			return;
		}

		getMemoryManager().lockEntry(rDescriptor.memoryEntryId);
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
		if(!rDescriptor.residency.loaded)
		{
			return false;
		}
		if(rDescriptor.memoryVisability != GPU_LOCAL || rDescriptor.firstDataSettled || !rDescriptor.gpuWritten)
		{
			return true;
		}

		if(getMemoryManager().hasPendingWrite(rDescriptor.memoryEntryId))
		{
			return false;
		}
		rDescriptor.firstDataSettled = true;
		return true;
	}
	bool DescriptorSet::canUnload() const
	{
		return this->allocatorType == DYNAMIC_ALLOCATOR;
	}

	bool DescriptorSet::unloadsAutomatically() const
	{
		return this->canUnload() && this->params.residency.absenceSecondsCount != UninitializedId;
	}

	bool DescriptorSet::entryHasShadow(const Descriptor& rDescriptor_) const
	{
		return rDescriptor_.isPerFrame() && Globals::renderer.framesAtFlightCount > 1;
	}

	bool DescriptorSet::mirrorsWrites(const Descriptor& rDescriptor_) const
	{
		return this->unloadsAutomatically() && this->params.residency.loadFromDisk == nullptr && !this->entryHasShadow(rDescriptor_);
	}

	void DescriptorSet::markDiskLayoutChanged(PoolId descriptorId_)
	{
		this->getDescriptor(descriptorId_).residency.diskLayoutStale = false;
	}

	MemoryEntryId DescriptorSet::getDescriptorEntryId(PoolId descriptorId_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		if(!rDescriptor.residency.loaded)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} is unloaded, it has no entry.", rDescriptor.getDescriptorLayout().name));
		}
		return rDescriptor.memoryEntryId;
	}

	uint64_t DescriptorSet::getLastUsedFrame(PoolId descriptorId_)
	{
		return this->getDescriptor(descriptorId_).residency.lastUsedFrame;
	}

	/* Stamps the use for the absence time (only a set that unloads by itself has one) and requests a load of an unloaded descriptor, also when it was unloaded by hand. A static set never leaves memory, so there is nothing to do. */
	void DescriptorSet::markUsed(PoolId descriptorId_)
	{
		if(!this->canUnload())
		{
			return;
		}

		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		rDescriptor.residency.markUsedFrame();
		if(this->unloadsAutomatically())
		{
			rDescriptor.residency.lastUsedTime = std::chrono::steady_clock::now();
		}
		if(!rDescriptor.residency.loaded)
		{
			rDescriptor.residency.loadRequested = true;
		}
	}

	/*
		Vulkan rule: a buffer may only be destroyed once no submitted command uses it (VUID-vkDestroyBuffer-buffer-00922), which the caller guarantees by waiting for the device. The binding is not rewritten: a descriptor whose buffer was destroyed is undefined, and that is legal as long as no shader that runs reads it (the draws and passes that use it are skipped or held until it is loaded again, and the validation layers report it when one slips through).
		The state that described the old entry is reset, the new one starts fresh (nothing written, not locked).
		The CPU copy of the bytes stays: an entry with a shadow hands it over (takeShadow) before it is removed, the others mirrored their writes already.
	*/
	void DescriptorSet::unloadDescriptor(PoolId descriptorId_)
	{
		if(!this->canUnload())
		{
			throw std::runtime_error("Descriptor Set: Only a DYNAMIC_ALLOCATOR set can unload, static entries can't be removed.");
		}

		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		if(!rDescriptor.residency.loaded)
		{
			return;
		}

		/*
			The bytes have to survive the removal of the entry, in one CPU copy: with loadFromDisk the file has them, an entry with a shadow hands the shadow over (takeShadow), a mirrored descriptor has the backup already, a CPU_SHARED entry is read (its memory is readable).
			A GPU_LOCAL entry without a shadow in a set that doesn't unload by itself has no copy and the GPU can't be read here, so it can't unload.
		*/
		if(this->params.residency.loadFromDisk == nullptr)
		{
			MemoryManager& rMemoryManager = getMemoryManager();
			if(this->entryHasShadow(rDescriptor))
			{
				rDescriptor.residency.backup.data = rMemoryManager.takeShadow(rDescriptor.memoryEntryId);
			}
			else if(!this->mirrorsWrites(rDescriptor))
			{
				if(rDescriptor.memoryVisability != CPU_SHARED)
				{
					throw std::runtime_error(std::format("Descriptor Set: {} can't unload, nothing could bring its bytes back. Create the set with residency.loadFromDisk or an absenceSecondsCount (the engine then keeps a CPU copy).", rDescriptor.getDescriptorLayout().name));
				}
				MemoryEntry& rEntry = rMemoryManager.getEntry(rDescriptor.memoryEntryId);
				rDescriptor.residency.backup.data.resize(rEntry.size);
				rMemoryManager.readShared(rEntry, 0, rDescriptor.residency.backup.data.data(), rEntry.size);
			}
		}
		getMemoryManager().removeDynamicEntry(rDescriptor.memoryEntryId);
		rDescriptor.residency.loaded = false;
		rDescriptor.residency.loadRequested = false;
		rDescriptor.created = false;
		rDescriptor.gpuWritten = false;
		rDescriptor.firstDataSettled = false;
		rDescriptor.readOnlyLocked = false;
	}

	/* Every VkDescriptorSet copy of the set is pointed at the descriptor's current entry with one vkUpdateDescriptorSets (no pending command buffer may use the set, see the header). */
	void DescriptorSet::rewriteBindings(Descriptor& rDescriptor_)
	{
		std::vector<VkDescriptorBufferInfo> bufferInfos(this->vkHandles.size());
		std::vector<VkWriteDescriptorSet> updateSetWrites;
		updateSetWrites.reserve(this->vkHandles.size());
		for(uint32_t copy = 0; copy < this->vkHandles.size(); copy++)
		{
			updateSetWrites.emplace_back(this->makeBindingWrite(rDescriptor_, copy, bufferInfos[copy]));
		}
		if(!updateSetWrites.empty())
		{
			vkUpdateDescriptorSets(getMainDevice().logicalDevice, updateSetWrites.size(), updateSetWrites.data(), 0, nullptr);
		}
	}

	/*
		A new entry, then every copy of the set is pointed at it with one vkUpdateDescriptorSets (no pending command buffer may use the set, see the header). The bytes come back from loadFromDisk or the CPU backup, as ordinary writes, so the descriptor is settled once their upload completed.
		loaded is set before the writes since the setters throw on an unloaded descriptor.
	*/
	void DescriptorSet::loadDescriptor(PoolId descriptorId_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		if(rDescriptor.residency.loaded)
		{
			return;
		}
		if(rDescriptor.residency.diskLayoutStale && this->params.residency.loadFromDisk != nullptr)
		{
			throw std::runtime_error(std::format("Descriptor Set: {} can't be loaded from disk, a layout update changed its block and the file still has the old layout. Call changeDescriptorLayoutOnDisk (and saveDescriptorToDisk) for it first.", rDescriptor.getDescriptorLayout().name));
		}

		this->allocateDescriptorMemory(rDescriptor);
		this->rewriteBindings(rDescriptor);

		rDescriptor.residency.loaded = true;
		rDescriptor.residency.loadRequested = false;
		rDescriptor.created = true;
		rDescriptor.residency.lastUsedTime = std::chrono::steady_clock::now();
		if(this->params.residency.loadFromDisk != nullptr)
		{
			this->params.residency.loadFromDisk(rDescriptor.setId, rDescriptor.DescriptorLayoutId);
		}
		else
		{
			this->restoreBackup(rDescriptor);
			/* Unless the writes are mirrored the bytes are in the new entry now (its shadow, or the CPU_SHARED memory), so the backup would be a second CPU copy of them. */
			if(!this->mirrorsWrites(rDescriptor))
			{
				std::vector<char>().swap(rDescriptor.residency.backup.data);
			}
		}
	}

	/*
		The block of the descriptor's descriptor layout changed (commitDescriptorLayoutUpdate swapped the members in, rOldLayout_ and rOldShape_ describe the block as it was). The caller waited for the device (the old entry is destroyed and the bindings rewritten).
		A loaded GPU_LOCAL descriptor without copies of a dynamic set gets a new entry that is filled on the GPU from the old one (moveDescriptorEntry), so bytes only the GPU holds survive. Any other loaded descriptor is unloaded, gets the new shape and is loaded again: the new entry holds the old bytes from the CPU backup remapped to the new offsets, or from whatever loadFromDisk writes.
		An unloaded one only gets the shape and the remapped backup. loadFromDisk is not called for it (it stays for the usual loads): the files keep the old layout until the application converts them (saveToDisk / changeLayoutOnDisk), so the descriptor is marked diskLayoutStale and a later load throws until changeDescriptorLayoutOnDisk was called for it (the load of this function, in the other path, still reads the old files, which is what the application converts from).
	*/
	void DescriptorSet::reshapeDescriptor(PoolId descriptorId_, const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		DescriptorLayout& rLayout = rDescriptor.getDescriptorLayout();
		DescriptorShape newShape = rLayout.makeShape(getDynamicAlignment(rDescriptor.getType()));

		/* Nothing the shader reads moved and the entry has the same size, e.g. a member that fit in the headroom was added: the entry (and its copies) stays, only the shape and the CPU copy change. */
		if(rDescriptor.residency.loaded && descriptorKeepsEntry(rOldLayout_, rOldShape_, rLayout, newShape))
		{
			rDescriptor.shape = newShape;
			this->validateShape(rDescriptor);
			if(!rDescriptor.residency.backup.data.empty())
			{
				rDescriptor.residency.backup.data = remapDescriptorBytes(rDescriptor.residency.backup.data, rOldLayout_, rOldShape_, rLayout, rDescriptor.shape);
			}
		}
		else if(rDescriptor.residency.loaded && this->canUnload() && rDescriptor.memoryVisability == GPU_LOCAL && !getMemoryManager().getEntry(rDescriptor.memoryEntryId).hasShadow)
		{
			this->moveDescriptorEntry(rDescriptor, rOldLayout_, rOldShape_, newShape);
		}
		else
		{
			bool wasLoaded = rDescriptor.residency.loaded;
			if(wasLoaded)
			{
				this->unloadDescriptor(descriptorId_);
			}

			rDescriptor.shape = newShape;
			this->validateShape(rDescriptor);
			if(!rDescriptor.residency.backup.data.empty())
			{
				rDescriptor.residency.backup.data = remapDescriptorBytes(rDescriptor.residency.backup.data, rOldLayout_, rOldShape_, rLayout, rDescriptor.shape);
			}

			if(wasLoaded)
			{
				this->loadDescriptor(descriptorId_);
			}
		}

		rDescriptor.residency.diskLayoutStale = this->params.residency.loadFromDisk != nullptr;
	}

	/*
		Vulkan rules: the old buffer is destroyed only once no submitted command uses it (VUID-vkDestroyBuffer-buffer-00922), which the device wait of the caller and the deferred removal (it waits for the copy that reads the old entry) guarantee, and the set is only updated while no pending command buffer uses it (VUID-vkUpdateDescriptorSets-None-03047), which the same wait guarantees.
		The new entry is filled by copyEntry (the variables both shapes have) and zeroEntryRanges (everything else), so it is pending until the copy completes: draws using the descriptor skip and it counts as unsettled, like after a load. The descriptor stays loaded, the state that described the old entry is reset like in unloadDescriptor.
		The CPU backup (kept only without loadFromDisk) is remapped as in a load.
	*/
	void DescriptorSet::moveDescriptorEntry(Descriptor& rDescriptor_, const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorShape& rNewShape_)
	{
		DescriptorLayout& rLayout = rDescriptor_.getDescriptorLayout();
		MemoryEntryId oldEntryId = rDescriptor_.memoryEntryId;
		rDescriptor_.shape = rNewShape_;
		this->validateShape(rDescriptor_);
		this->allocateDescriptorMemory(rDescriptor_);

		MemoryManager& rMemoryManager = getMemoryManager();
		rMemoryManager.copyEntry(oldEntryId, rDescriptor_.memoryEntryId, makeDescriptorCopyRegions(rOldLayout_, rOldShape_, rLayout, rDescriptor_.shape));
		rMemoryManager.zeroEntryRanges(rDescriptor_.memoryEntryId, makeDescriptorZeroRegions(rOldLayout_, rOldShape_, rLayout, rDescriptor_.shape));
		rMemoryManager.removeDynamicEntry(oldEntryId);
		this->rewriteBindings(rDescriptor_);

		rDescriptor_.gpuWritten = true;
		rDescriptor_.firstDataSettled = false;
		rDescriptor_.readOnlyLocked = false;
		if(!rDescriptor_.residency.backup.data.empty())
		{
			rDescriptor_.residency.backup.data = remapDescriptorBytes(rDescriptor_.residency.backup.data, rOldLayout_, rOldShape_, rLayout, rDescriptor_.shape);
		}
	}

	/*
		A descriptor layout was committed to the set's layout: the set gets its descriptor in the same slot (descriptors mirror the slots of the descriptor layouts). If the memory of the set exists, the new descriptor gets its entry too, which a static set can't (static entries have to be added before the static allocation).
		The VkDescriptorSets don't have the binding yet, rebuildHandles writes it.
	*/
	void DescriptorSet::addDescriptor(PoolId setId_, PoolId descriptorLayoutId_)
	{
		PoolId descriptorId = this->descriptors.addAt(descriptorLayoutId_.id, this->setLayout, this->params.memoryVisability, setId_, descriptorLayoutId_);
		this->descriptors.getInternal(descriptorId.id).getDescriptorLayout().derivedDescriptors.emplace_back(setId_);
		if(this->descriptorsMemoryAllocated)
		{
			this->setupDescriptorMemory(this->descriptors.getInternal(descriptorId.id));
			this->checkDynamicLimits();
		}
	}

	/* The entries of a loaded descriptor of a dynamic set are removed, a static entry can't be and stays in the static heap until shutdown (like removeDescriptorSet). */
	void DescriptorSet::removeDescriptor(PoolId descriptorLayoutId_)
	{
		Descriptor& rDescriptor = this->descriptors.getInternal(descriptorLayoutId_.id);
		if(this->descriptorsMemoryAllocated && this->allocatorType == DYNAMIC_ALLOCATOR && rDescriptor.residency.loaded)
		{
			getMemoryManager().removeDynamicEntry(rDescriptor.memoryEntryId);
		}
		std::erase(rDescriptor.getDescriptorLayout().derivedDescriptors, rDescriptor.setId);
		this->descriptors.removeInternal(descriptorLayoutId_.id);
	}

	/*
		A set belongs to the VkDescriptorSetLayout it was allocated with, so a recreated layout needs new VkDescriptorSets. The old ones went with the old layout's pools (DescriptorSetLayout::destroy), so there is nothing to free.
		create allocates them again and writes the bindings of the loaded descriptors.
	*/
	void DescriptorSet::rebuildHandles()
	{
		this->vkHandles.clear();
		this->poolId = UninitializedId;
		this->create();
	}

	/* One write of the whole entry, so a GPU_LOCAL descriptor is one update (one staging range) and not one per variable. */
	void DescriptorSet::restoreBackup(Descriptor& rDescriptor_)
	{
		uint64_t size = rDescriptor_.residency.backup.data.size();
		if(size == 0)
		{
			return;
		}

		if(rDescriptor_.memoryVisability == GPU_LOCAL)
		{
			rDescriptor_.gpuWritten = true;
			StagingDataContainer container = this->getStagingContainer(rDescriptor_.memoryEntryId, 0, size);
			container.write(0, rDescriptor_.residency.backup.data.data(), size);
		}
		else
		{
			SharedDataContainer container = this->getSharedContainer(rDescriptor_.memoryEntryId);
			container.validFirstOffset = 0;
			container.validSize = size;
			container.write(0, rDescriptor_.residency.backup.data.data(), size);
		}
	}
}
