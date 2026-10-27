#pragma once 
#include "GPUDataContainer.h"
#include "MemoryEntryId.h"

namespace Graphics 
{
struct Descriptor 
{
	uint32_t DSlayoutId; 
	PoolId setId; 
	PoolId BlueprintId; 
	MemoryEntryId memoryEntryId; 

	bool created = false; 
};
}
