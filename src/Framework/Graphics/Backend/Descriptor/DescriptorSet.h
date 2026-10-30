// Pointer table for shaders to connect to In Memory GPU Resources 
#pragma once 
#include "MemoryEntryId.h"
#include "Descriptor.h"
#include "DescriptorPool.h"
#include "GPUDataContainer.h"
#include "Layout.h"
#include "Pool.h"
#include "Utilities.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics 
{
struct DescriptorSet
{
	PoolId setLayout; 
	DescriptorTypeEnum descriptorType; // inhereted from the set layout 
	AllocatorTypeEnum allocatorType; // allocator of the memory entries of descriptors 
	DescriptorPool pool; // its 1 to 1 due to dynamic buffers. That way the logic is much simpler. 
	/* One per frame in flight if any descriptor isPerFrame(), otherwise one. */
	std::vector<VkDescriptorSet> vkHandles;
	Pool<Descriptor> descriptors; // per binding

	VkDescriptorSet get() const;
	bool descriptorsMemoryAllocated = false;

	DescriptorSet(PoolId setLayout_, AllocatorTypeEnum allocatorType_);

	/* 
		Writes one member of the descriptor. descriptorId_ is the id of the blueprint the descriptor was derived from (they share the slot). Only valid after allocateDescriptorsMemory.
		GPU_LOCAL descriptors are written as updates through staging. A per-frame descriptor (Descriptor::isPerFrame) only writes its shadow and bumps its version, so it never waits and may be called at any time, even while recording: syncToFrame copies it into the frame's entry. Other CPU_SHARED descriptors (readOnly) are written directly into the mapped memory without waiting: they are only legal until the submit that first recorded a draw reading them (lockReadOnlyDescriptor), a write to a readOnly descriptor after that throws.
	*/
	template<typename T>
	void setDescriptorMemberValue(PoolId descriptorId_, PoolId memberId_, T data_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		ReflectionLayout& rLayout = this->getReflectionLayout(rDescriptor);
		const MemberBlueprint& rMember = rLayout.memberBlueprints[memberId_];

		if(rDescriptor.memoryVisability == GPU_LOCAL)
		{
			rDescriptor.gpuWritten = true;
			StagingDataContainer container = this->getStagingContainer(rDescriptor.memoryEntryIds[0], rMember.firstByteId, rMember.size);
			rLayout.setMember(container, memberId_, 0, data_);
		}
		else if(rDescriptor.isPerFrame())
		{
			rLayout.setMember(rDescriptor.shadow, memberId_, 0, data_);
			rDescriptor.version++;
		}
		else
		{
			SharedDataContainer container = this->getSharedContainer(rDescriptor.memoryEntryIds[0]);
			container.validFirstOffset = rMember.firstByteId;
			container.validSize = rMember.size;
			rLayout.setMember(container, memberId_, 0, data_);
		}
	}
	
	/* Has to be called before allocateDescriptorsMemory, which passes the flag to the memory entry. Throws afterwards. */
	void setDescriptorReadOnly(PoolId descriptorId_, bool readOnly_);
	void allocateDescriptorsMemory(); // before StaticAllocator::allocateAndUpload, since static entries get their buffer there
	void create(); // after the memory buffers exist 
	void destroy();
	void lockReadOnlyDescriptor(PoolId descriptorId_); // locks the entries of the descriptor if it is readOnly, once. Called by DescriptorsManager::lockBoundDescriptors after the submit that first recorded a draw reading it
	bool isDescriptorSettled(PoolId descriptorId_); // true unless the descriptor is GPU_LOCAL and its first write is still pending, so a draw may read it. drawMeshIndexed skips the draws of shaders that use it otherwise
	void syncToFrame(uint32_t frame_); // copies the shadows that changed into frame_'s entries, see the .cpp

	private:
	Descriptor& getDescriptor(PoolId descriptorId_);
	ReflectionLayout& getReflectionLayout(Descriptor& rDescriptor_);
	StagingDataContainer getStagingContainer(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_);
	SharedDataContainer getSharedContainer(MemoryEntryId entryId_);
};
} 
