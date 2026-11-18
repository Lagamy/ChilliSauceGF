#pragma once 
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
	/* A per frame descriptor's entry has one copy per frame in flight (MemoryEntry::copyCount), the writes go to the entry's shadow and MemoryManager::syncCopiesToFrame refreshes the copies. */
	MemoryEntryId memoryEntryId;
	/* Built in allocateDescriptorsMemory from the descriptor layout (DescriptorLayout::makeShape). Sizes the entry (every copy has the same size), places every member in the block, and the dynamic range and offsets are read from it. */
	DescriptorShape shape;

	bool created = false;
	bool readOnly = false;
	bool gpuWritten = false; // setDescriptorMemberValue wrote a GPU_LOCAL descriptor at least once
	bool firstDataSettled = false; // a check saw no pending write after the first one, see DescriptorSet::isDescriptorSettled
	bool readOnlyLocked = false; // the entry was locked, see DescriptorSet::lockReadOnlyDescriptor

	Descriptor(PoolId DSlayoutId_, MemoryVisabilityEnum memoryVisability_, PoolId setId_, PoolId DescriptorLayoutId_) : DSlayoutId(DSlayoutId_), memoryVisability(memoryVisability_), setId(setId_), DescriptorLayoutId(DescriptorLayoutId_) {};
	DescriptorLayout& getDescriptorLayout();
	bool isPerFrame() const;
	bool isDynamic(); // the block is picked per bind (dynamic offset), from the descriptor layout
	DescriptorTypeEnum getType(); // UNIFORM_DESCRIPTOR / STORAGE_DESCRIPTOR..., from the descriptor layout
};
}
