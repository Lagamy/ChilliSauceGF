#pragma once 
#include "CPUDataContainer.h"
#include "GPUDataContainer.h"
#include "MemoryEntryId.h"
#include "DescriptorLayout.h"
#include "Utilities.h"
#include "DescriptorShape.h"
#include <vector>

namespace Graphics 
{
struct Descriptor 
{	
	PoolId DSlayoutId;
	MemoryVisabilityEnum memoryVisability; 
	PoolId setId; 
	PoolId DescriptorLayoutId; 
	/* One entry per frame in flight if isPerFrame(), otherwise a single shared one. */
	std::vector<MemoryEntryId> memoryEntryIds;
	/* Built in allocateDescriptorsMemory from the descriptor layout (DescriptorLayout::makeShape). Sizes the entries and the shadow (every copy and the shadow have the same size), places every member in the block, and the dynamic range and offsets are read from it. */
	DescriptorShape shape;
	/* Only for isPerFrame(): the CPU copy of the entry's bytes (shape.getSize() bytes). Writes go here at any time, syncToFrame copies it into the frame's entry. */
	CPUDataContainer shadow;
	/* Counts the writes to the shadow. Starts at 1, so the first syncToFrame fills every copy. */
	uint64_t version = 1;
	/* Per frame: the version of the shadow that frame's entry holds. Starts at 0. */
	std::vector<uint64_t> copyVersions;

	bool created = false;
	bool readOnly = false;
	bool gpuWritten = false; // setDescriptorMemberValue wrote a GPU_LOCAL descriptor at least once
	bool firstDataSettled = false; // a check saw no pending write after the first one, see DescriptorSet::isDescriptorSettled
	bool readOnlyLocked = false; // the entries were locked, see DescriptorSet::lockReadOnlyDescriptor

	Descriptor(PoolId DSlayoutId_, MemoryVisabilityEnum memoryVisability_, PoolId setId_, PoolId DescriptorLayoutId_) : DSlayoutId(DSlayoutId_), memoryVisability(memoryVisability_), setId(setId_), DescriptorLayoutId(DescriptorLayoutId_) {};
	DescriptorLayout& getDescriptorLayout();
	bool isPerFrame() const;
	bool isDynamic(); // the block is picked per bind (dynamic offset), from the descriptor layout
	DescriptorTypeEnum getType(); // UNIFORM_DESCRIPTOR / STORAGE_DESCRIPTOR..., from the descriptor layout
	MemoryEntryId getEntryId(uint32_t frame_) const;
};
}
