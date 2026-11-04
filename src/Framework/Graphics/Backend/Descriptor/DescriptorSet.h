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
#include <format>
#include <stdexcept>
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
		The functions below write a part of an ARRAY / MATRIX member the same way. Each GPU_LOCAL write is its own staging range and update, covering only the bytes it replaces (an element, a row), so a column is written element by element.
	*/
	template<typename T>
	void setDescriptorMemberValue(PoolId descriptorId_, PoolId memberId_, T data_)
	{
		const MemberBlueprint& rMember = this->getMember(descriptorId_, memberId_);
		this->writeMemberRange(descriptorId_, memberId_, 0, rMember.size, [&](ReflectionLayout& rLayout_, auto& rContainer_)
		{
			rLayout_.setMemberScalar(rContainer_, memberId_, 0, data_);
		});
	}

	template<typename T>
	void setDescriptorMemberArrayElement(PoolId descriptorId_, PoolId memberId_, uint32_t elementId_, T data_)
	{
		const MemberBlueprint& rMember = this->getMember(descriptorId_, memberId_);
		this->writeMemberRange(descriptorId_, memberId_, rMember.getArrayElementOffset(elementId_), rMember.getElementStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
		{
			rLayout_.setElementInMemberArray(rContainer_, memberId_, 0, elementId_, data_);
		});
	}

	template<typename T>
	void setDescriptorMemberMatrixElement(PoolId descriptorId_, PoolId memberId_, uint32_t rowId_, uint32_t columnId_, T data_)
	{
		const MemberBlueprint& rMember = this->getMember(descriptorId_, memberId_);
		this->writeMemberRange(descriptorId_, memberId_, rMember.getMatrixElementOffset(rowId_, columnId_), rMember.getElementStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
		{
			rLayout_.setElementInMemberMatrix(rContainer_, memberId_, 0, rowId_, columnId_, data_);
		});
	}

	/* Replaces a whole row. The Array must have one element per column of the member. */
	template<typename T>
	void setDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId memberId_, uint32_t rowId_, const Array<T>& rSrcArray_)
	{
		const MemberBlueprint& rMember = this->getMember(descriptorId_, memberId_);
		this->writeMemberRange(descriptorId_, memberId_, rMember.getMatrixElementOffset(rowId_, 0), rMember.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, memberId_, 0, rowId_, rSrcArray_);
		});
	}

	/* Replaces a whole row with row srcRowId_ of a Matrix. The Matrix must have as many columns as the member. */
	template<typename T>
	void setDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId memberId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_)
	{
		const MemberBlueprint& rMember = this->getMember(descriptorId_, memberId_);
		this->writeMemberRange(descriptorId_, memberId_, rMember.getMatrixElementOffset(rowId_, 0), rMember.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, memberId_, 0, rowId_, rSrcMatrix_, srcRowId_);
		});
	}

	/* Replaces a whole column. The Array must have one element per row of the member. */
	template<typename T>
	void setDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId memberId_, uint32_t columnId_, const Array<T>& rSrcArray_)
	{
		const MemberBlueprint& rMember = this->getMember(descriptorId_, memberId_);
		if(rSrcArray_.size() != rMember.shapeMetadata.rowCount)
		{
			throw std::runtime_error(std::format("Descriptor Set: Array with {} elements can't replace a column of {} elements.", rSrcArray_.size(), rMember.shapeMetadata.rowCount));
		}
		for(uint32_t rowId = 0; rowId < rSrcArray_.size(); rowId++)
		{
			this->setDescriptorMemberMatrixElement(descriptorId_, memberId_, rowId, columnId_, rSrcArray_.getElement(rowId));
		}
	}

	/* Replaces a whole column with column srcColumnId_ of a Matrix. The Matrix must have as many rows as the member. */
	template<typename T>
	void setDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId memberId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_)
	{
		const MemberBlueprint& rMember = this->getMember(descriptorId_, memberId_);
		if(rSrcMatrix_.getRowCount() != rMember.shapeMetadata.rowCount || srcColumnId_ >= rSrcMatrix_.getColumnCount())
		{
			throw std::runtime_error(std::format("Descriptor Set: Column {} of a {}x{} Matrix can't replace a column of {} elements.", srcColumnId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rMember.shapeMetadata.rowCount));
		}
		for(uint32_t rowId = 0; rowId < rSrcMatrix_.getRowCount(); rowId++)
		{
			this->setDescriptorMemberMatrixElement(descriptorId_, memberId_, rowId, columnId_, rSrcMatrix_.getElement(rowId, srcColumnId_));
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
	const MemberBlueprint& getMember(PoolId descriptorId_, PoolId memberId_);

	/*
		Calls write_(layout, container) with the container the descriptor is written through, covering the bytes [firstByteId + inMemberOffset_, + size_) of the member's entry.
		See setDescriptorMemberValue for the three cases.
	*/
	template<typename F>
	void writeMemberRange(PoolId descriptorId_, PoolId memberId_, uint64_t inMemberOffset_, uint64_t size_, F&& write_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		ReflectionLayout& rLayout = this->getReflectionLayout(rDescriptor);
		uint64_t inEntryOffset = rLayout.memberBlueprints[memberId_].firstByteId + inMemberOffset_;

		if(rDescriptor.memoryVisability == GPU_LOCAL)
		{
			rDescriptor.gpuWritten = true;
			StagingDataContainer container = this->getStagingContainer(rDescriptor.memoryEntryIds[0], inEntryOffset, size_);
			write_(rLayout, container);
		}
		else if(rDescriptor.isPerFrame())
		{
			write_(rLayout, rDescriptor.shadow);
			rDescriptor.version++;
		}
		else
		{
			SharedDataContainer container = this->getSharedContainer(rDescriptor.memoryEntryIds[0]);
			container.validFirstOffset = inEntryOffset;
			container.validSize = size_;
			write_(rLayout, container);
		}
	}

	ReflectionLayout& getReflectionLayout(Descriptor& rDescriptor_);
	StagingDataContainer getStagingContainer(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_);
	SharedDataContainer getSharedContainer(MemoryEntryId entryId_);
};
} 
