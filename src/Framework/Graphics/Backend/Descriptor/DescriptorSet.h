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
#include <vulkan/vulkan.h>

namespace Graphics 
{
struct DescriptorSet
{
	PoolId setLayout; 
	DescriptorTypeEnum descriptorType; // inhereted from the set layout 
	AllocatorTypeEnum allocatorType; // allocator of the memory entries of descriptors 
	DescriptorPool pool; // its 1 to 1 due to dynamic buffers. That way the logic is much simpler. 
	VkDescriptorSet vkHandle = VK_NULL_HANDLE;
	Pool<Descriptor> descriptors; // per binding

	VkDescriptorSet get() const;
	bool descriptorsMemoryAllocated = false;
	/* Bit f: the command buffer recorded for frame f binds this set and frame f's fence wasn't waited on since. */
	uint32_t framesInFlightMask = 0;

	DescriptorSet(PoolId setLayout_, AllocatorTypeEnum allocatorType_);

	/* 
		Writes one member of the descriptor. descriptorId_ is the id of the blueprint the descriptor was derived from (they share the slot). Only valid after allocateDescriptorsMemory.
		GPU_LOCAL descriptors are written as updates through staging, CPU_SHARED ones directly into the mapped memory after the frames that bind this set are done (waitForFramesUsingSet).
		Must not be called while a command buffer is being recorded: the current frame's fence is reset but not submitted yet, so the wait would never return.
	*/
	template<typename T>
	void setDescriptorMemberValue(PoolId descriptorId_, PoolId memberId_, T data_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		ReflectionLayout& rLayout = this->getReflectionLayout(rDescriptor);
		const MemberBlueprint& rMember = rLayout.memberBlueprints[memberId_];

		if(rDescriptor.memoryVisability == GPU_LOCAL)
		{
			StagingDataContainer container = this->getStagingContainer(rDescriptor, rMember.firstByteId, rMember.size);
			rLayout.setMember(container, memberId_, 0, data_);
		}
		else
		{
			this->waitForFramesUsingSet();
			SharedDataContainer container = this->getSharedContainer(rDescriptor);
			container.validFirstOffset = rMember.firstByteId;
			container.validSize = rMember.size;
			rLayout.setMember(container, memberId_, 0, data_);
		}
	}
	
	void allocateDescriptorsMemory(); // before StaticAllocator::allocateAndUpload, since static entries get their buffer there
	void create(); // after the memory buffers exist 
	void destroy();
	void markUsedInCurrentFrame();

	private:
	void waitForFramesUsingSet();
	Descriptor& getDescriptor(PoolId descriptorId_);
	ReflectionLayout& getReflectionLayout(Descriptor& rDescriptor_);
	StagingDataContainer getStagingContainer(const Descriptor& rDescriptor_, uint64_t inEntryOffset_, uint64_t size_);
	SharedDataContainer getSharedContainer(const Descriptor& rDescriptor_);
};
} 
