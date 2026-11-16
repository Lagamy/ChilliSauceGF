// Pointer table for shaders to connect to In Memory GPU Resources 
#pragma once 
#include "MemoryEntryId.h"
#include "Descriptor.h"
#include "GPUDataContainer.h"
#include "Layout.h"
#include "Pool.h"
#include "Utilities.h"
#include <cstdint>
#include <span>
#include <format>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics 
{
struct DescriptorSet
{
	PoolId setLayout; 
	AllocatorTypeEnum allocatorType; // allocator of the memory entries of descriptors 
	/* One per frame in flight if any descriptor isPerFrame(), otherwise one. */
	std::vector<VkDescriptorSet> vkHandles;
	uint32_t poolId = UninitializedId; // index into the layout's pools of the pool vkHandles came from
	Pool<Descriptor> descriptors; // per binding

	VkDescriptorSet get() const;

	/*
		Dynamic offsets (bytes) for vkCmdBindDescriptorSets, from the repeat unit each dynamic descriptor should see in the bind. unitIds_ holds one id per dynamic descriptor, in binding order (Vulkan orders dynamic offsets by binding). Only the dynamic descriptors take part, so it is empty for a set without any. Throws on a wrong count or a unit id >= repeatCount.
	*/
	std::vector<uint32_t> getDynamicOffsets(std::span<const uint32_t> unitIds_);
	bool descriptorsMemoryAllocated = false;

	DescriptorSet(PoolId setLayout_, AllocatorTypeEnum allocatorType_);

	/* 
		Writes one variable of the descriptor: variableId_ is the id addMemberBlueprint / addVariable returned. A variable of a struct member (Object states[2]) takes one id per repeat layer of the struct (repeatUnitId_, 0 by default, is the one-layer case, or one id per layer through the std::vector<uint32_t> overloads), a variable directly in the block takes none. Those are for descriptors without a dynamic layer: a dynamic descriptor layout (addBufferDynamicDescriptorLayout) is written through the setDynamicDescriptorMember... variants, which take the dynamic unit id (the block, padded to the offset alignment) and optionally the ids of the struct's layers. descriptorId_ is the id of the descriptor layout the descriptor was derived from (they share the slot). Only valid after allocateDescriptorsMemory.
		GPU_LOCAL descriptors are written as updates through staging. A per-frame descriptor (Descriptor::isPerFrame) only writes its shadow and bumps its version, so it never waits and may be called at any time, even while recording: syncToFrame copies it into the frame's entry. Other CPU_SHARED descriptors (readOnly) are written directly into the mapped memory without waiting: they are only legal until the submit that first recorded a draw reading them (lockReadOnlyDescriptor), a write to a readOnly descriptor after that throws.
		A variable declared with repeatCountPerDimension (int a[2][3]) takes repeatIdPerDimension_ in every setter, one id per dimension, outermost first (empty for a variable that isn't repeated), after the struct ids.
		The functions below write a part of a MATRIX variable the same way. Each GPU_LOCAL write is its own staging range and update, covering only the bytes it replaces (an element, a row), so a column is written element by element.
	*/
	template<typename T>
	void setDescriptorMemberValue(PoolId descriptorId_, PoolId variableId_, T data_, const std::vector<uint32_t>& idsPerLayer_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, idsPerLayer_, false, rVariable.getRepeatOffset(repeatIdPerDimension_), rVariable.elementSize, [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setMemberScalar(rContainer_, inLayoutId_, 0, data_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDynamicDescriptorMemberValue(PoolId descriptorId_, PoolId variableId_, T data_, uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIdsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, this->makeDynamicIds(dynamicUnitId_, innerIdsPerLayer_), true, rVariable.getRepeatOffset(repeatIdPerDimension_), rVariable.elementSize, [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setMemberScalar(rContainer_, inLayoutId_, 0, data_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDescriptorMemberValue(PoolId descriptorId_, PoolId variableId_, T data_, uint32_t repeatUnitId_ = 0, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		this->setDescriptorMemberValue(descriptorId_, variableId_, data_, this->makeSingleLayerIds(descriptorId_, variableId_, repeatUnitId_), repeatIdPerDimension_);
	}

	template<typename T>
	void setDescriptorMemberMatrixElement(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, uint32_t columnId_, T data_, const std::vector<uint32_t>& idsPerLayer_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, idsPerLayer_, false, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, columnId_), rVariable.getElementStride(), [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setElementInMemberMatrix(rContainer_, inLayoutId_, 0, rowId_, columnId_, data_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDynamicDescriptorMemberMatrixElement(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, uint32_t columnId_, T data_, uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIdsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, this->makeDynamicIds(dynamicUnitId_, innerIdsPerLayer_), true, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, columnId_), rVariable.getElementStride(), [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setElementInMemberMatrix(rContainer_, inLayoutId_, 0, rowId_, columnId_, data_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDescriptorMemberMatrixElement(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, uint32_t columnId_, T data_, uint32_t repeatUnitId_ = 0, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		this->setDescriptorMemberMatrixElement(descriptorId_, variableId_, rowId_, columnId_, data_, this->makeSingleLayerIds(descriptorId_, variableId_, repeatUnitId_), repeatIdPerDimension_);
	}

	/* Replaces a whole row. The Array must have one element per column of the member. */
	template<typename T>
	void setDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& idsPerLayer_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, idsPerLayer_, false, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, inLayoutId_, 0, rowId_, rSrcArray_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDynamicDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, const Array<T>& rSrcArray_, uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIdsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, this->makeDynamicIds(dynamicUnitId_, innerIdsPerLayer_), true, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, inLayoutId_, 0, rowId_, rSrcArray_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, const Array<T>& rSrcArray_, uint32_t repeatUnitId_ = 0, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		this->setDescriptorMemberMatrixRow(descriptorId_, variableId_, rowId_, rSrcArray_, this->makeSingleLayerIds(descriptorId_, variableId_, repeatUnitId_), repeatIdPerDimension_);
	}

	/* Replaces a whole row with row srcRowId_ of a Matrix. The Matrix must have as many columns as the member. */
	template<typename T>
	void setDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_, const std::vector<uint32_t>& idsPerLayer_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, idsPerLayer_, false, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, inLayoutId_, 0, rowId_, rSrcMatrix_, srcRowId_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDynamicDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_, uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIdsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		this->writeMemberRange(descriptorId_, variableId_, this->makeDynamicIds(dynamicUnitId_, innerIdsPerLayer_), true, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, PoolId inLayoutId_, auto& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, inLayoutId_, 0, rowId_, rSrcMatrix_, srcRowId_, repeatIdPerDimension_);
		});
	}

	template<typename T>
	void setDescriptorMemberMatrixRow(PoolId descriptorId_, PoolId variableId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_, uint32_t repeatUnitId_ = 0, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		this->setDescriptorMemberMatrixRow(descriptorId_, variableId_, rowId_, rSrcMatrix_, srcRowId_, this->makeSingleLayerIds(descriptorId_, variableId_, repeatUnitId_), repeatIdPerDimension_);
	}

	/* Replaces a whole column. The Array must have one element per row of the member. */
	template<typename T>
	void setDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId variableId_, uint32_t columnId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& idsPerLayer_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		if(rSrcArray_.size() != rVariable.shapeMetadata.rowCount)
		{
			throw std::runtime_error(std::format("Descriptor Set: Array with {} elements can't replace a column of {} elements.", rSrcArray_.size(), rVariable.shapeMetadata.rowCount));
		}
		for(uint32_t rowId = 0; rowId < rSrcArray_.size(); rowId++)
		{
			this->setDescriptorMemberMatrixElement(descriptorId_, variableId_, rowId, columnId_, rSrcArray_.getElement(rowId), idsPerLayer_, repeatIdPerDimension_);
		}
	}

	template<typename T>
	void setDynamicDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId variableId_, uint32_t columnId_, const Array<T>& rSrcArray_, uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIdsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		if(rSrcArray_.size() != rVariable.shapeMetadata.rowCount)
		{
			throw std::runtime_error(std::format("Descriptor Set: Array with {} elements can't replace a column of {} elements.", rSrcArray_.size(), rVariable.shapeMetadata.rowCount));
		}
		for(uint32_t rowId = 0; rowId < rSrcArray_.size(); rowId++)
		{
			this->setDynamicDescriptorMemberMatrixElement(descriptorId_, variableId_, rowId, columnId_, rSrcArray_.getElement(rowId), dynamicUnitId_, innerIdsPerLayer_, repeatIdPerDimension_);
		}
	}

	template<typename T>
	void setDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId variableId_, uint32_t columnId_, const Array<T>& rSrcArray_, uint32_t repeatUnitId_ = 0, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		this->setDescriptorMemberMatrixColumn(descriptorId_, variableId_, columnId_, rSrcArray_, this->makeSingleLayerIds(descriptorId_, variableId_, repeatUnitId_), repeatIdPerDimension_);
	}

	/* Replaces a whole column with column srcColumnId_ of a Matrix. The Matrix must have as many rows as the member. */
	template<typename T>
	void setDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId variableId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_, const std::vector<uint32_t>& idsPerLayer_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		if(rSrcMatrix_.getRowCount() != rVariable.shapeMetadata.rowCount || srcColumnId_ >= rSrcMatrix_.getColumnCount())
		{
			throw std::runtime_error(std::format("Descriptor Set: Column {} of a {}x{} Matrix can't replace a column of {} elements.", srcColumnId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rVariable.shapeMetadata.rowCount));
		}
		for(uint32_t rowId = 0; rowId < rSrcMatrix_.getRowCount(); rowId++)
		{
			this->setDescriptorMemberMatrixElement(descriptorId_, variableId_, rowId, columnId_, rSrcMatrix_.getElement(rowId, srcColumnId_), idsPerLayer_, repeatIdPerDimension_);
		}
	}

	template<typename T>
	void setDynamicDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId variableId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_, uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIdsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		const VariableBlueprint& rVariable = this->getVariable(descriptorId_, variableId_);
		if(rSrcMatrix_.getRowCount() != rVariable.shapeMetadata.rowCount || srcColumnId_ >= rSrcMatrix_.getColumnCount())
		{
			throw std::runtime_error(std::format("Descriptor Set: Column {} of a {}x{} Matrix can't replace a column of {} elements.", srcColumnId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rVariable.shapeMetadata.rowCount));
		}
		for(uint32_t rowId = 0; rowId < rSrcMatrix_.getRowCount(); rowId++)
		{
			this->setDynamicDescriptorMemberMatrixElement(descriptorId_, variableId_, rowId, columnId_, rSrcMatrix_.getElement(rowId, srcColumnId_), dynamicUnitId_, innerIdsPerLayer_, repeatIdPerDimension_);
		}
	}

	template<typename T>
	void setDescriptorMemberMatrixColumn(PoolId descriptorId_, PoolId variableId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_, uint32_t repeatUnitId_ = 0, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
	{
		this->setDescriptorMemberMatrixColumn(descriptorId_, variableId_, columnId_, rSrcMatrix_, srcColumnId_, this->makeSingleLayerIds(descriptorId_, variableId_, repeatUnitId_), repeatIdPerDimension_);
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
	void validateShape(Descriptor& rDescriptor_); // the uniform range limit, see the .cpp
	const VariableBlueprint& getVariable(PoolId descriptorId_, PoolId variableId_);
	std::vector<uint32_t> makeSingleLayerIds(PoolId descriptorId_, PoolId variableId_, uint32_t repeatUnitId_);
	/* Plain setters are for descriptors without a dynamic layer, setDynamic... ones for dynamic descriptor layouts. Throws on a mismatch. */
	static void checkDynamic(Descriptor& rDescriptor_, bool dynamic_);
	static std::vector<uint32_t> makeDynamicIds(uint32_t dynamicUnitId_, const std::vector<uint32_t>& innerIds_);

	/*
		Calls write_(layout, variableIdInLayout, container) with the container the descriptor is written through, covering the bytes [firstByteId + inVariableOffset_, + size_) of the variable inside its entry.
		The variable's offset is the dynamic unit (dynamic_: the first id of idsPerLayer_, times the padded stride) plus the member's place in the block plus the struct element picked by the remaining ids (one per repeat layer of its struct, none for a variable directly in the block).
		See setDescriptorMemberValue for the three cases.
	*/
	template<typename F>
	void writeMemberRange(PoolId descriptorId_, PoolId variableId_, const std::vector<uint32_t>& idsPerLayer_, bool dynamic_, uint64_t inVariableOffset_, uint64_t size_, F&& write_)
	{
		Descriptor& rDescriptor = this->getDescriptor(descriptorId_);
		checkDynamic(rDescriptor, dynamic_);
		DescriptorLayout& rDescriptorLayout = rDescriptor.getDescriptorLayout();
		const VariableRef& rVariableRef = rDescriptorLayout.getVariable(variableId_);
		ReflectionLayout& rLayout = rDescriptorLayout.getMember(rVariableRef.memberId).layout;
		const MemberPlacement& rPlacement = rDescriptor.shape.placements[rVariableRef.memberId.id];

		std::span<const uint32_t> structIds = idsPerLayer_;
		uint64_t dynamicOffset = 0;
		if(dynamic_)
		{
			if(structIds.empty())
			{
				throw std::runtime_error(std::format("Descriptor Set: {} needs a dynamic unit id.", rDescriptorLayout.name));
			}
			RepeatLayer dynamicLayer = rDescriptor.shape.dynamicLayer;
			dynamicOffset = computeRepeatOffset(std::span<const RepeatLayer>(&dynamicLayer, 1), structIds.first(1));
			structIds = structIds.subspan(1);
		}

		/* Throws on a wrong number of ids or an id out of range of its layer. The layout always writes at unit 0, the element's offset travels in repeatBaseOffset. */
		uint64_t repeatOffset = dynamicOffset + rPlacement.firstByteId + rPlacement.shape.getOffset(structIds);
		uint64_t inEntryOffset = repeatOffset + rLayout.variableBlueprints[rVariableRef.variableId].firstByteId + inVariableOffset_;

		if(rDescriptor.memoryVisability == GPU_LOCAL)
		{
			rDescriptor.gpuWritten = true;
			StagingDataContainer container = this->getStagingContainer(rDescriptor.memoryEntryIds[0], inEntryOffset, size_);
			container.repeatCount = 1;
			container.repeatStride = rLayout.size;
			container.repeatBaseOffset = repeatOffset;
			write_(rLayout, rVariableRef.variableId, container);
		}
		else if(rDescriptor.isPerFrame())
		{
			rDescriptor.shadow.repeatCount = 1;
			rDescriptor.shadow.repeatStride = rLayout.size;
			rDescriptor.shadow.repeatBaseOffset = repeatOffset;
			write_(rLayout, rVariableRef.variableId, rDescriptor.shadow);
			rDescriptor.version++;
		}
		else
		{
			SharedDataContainer container = this->getSharedContainer(rDescriptor.memoryEntryIds[0]);
			container.validFirstOffset = inEntryOffset;
			container.validSize = size_;
			container.repeatCount = 1;
			container.repeatStride = rLayout.size;
			container.repeatBaseOffset = repeatOffset;
			write_(rLayout, rVariableRef.variableId, container);
		}
	}

	StagingDataContainer getStagingContainer(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_);
	SharedDataContainer getSharedContainer(MemoryEntryId entryId_);
};
} 
