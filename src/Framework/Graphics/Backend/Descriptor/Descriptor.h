#pragma once 
#include "GPUDataContainer.h"
#include "MemoryEntryId.h"
#include "DescriptorBlueprint.h"
#include "Utilities.h"

namespace Graphics 
{
struct Descriptor 
{	
	PoolId DSlayoutId;
	MemoryVisabilityEnum memoryVisability; 
	PoolId setId; 
	PoolId BlueprintId; 
	MemoryEntryId memoryEntryId; 

	bool created = false;

	Descriptor(PoolId DSlayoutId_, MemoryVisabilityEnum memoryVisability_, PoolId setId_, PoolId BlueprintId_) : DSlayoutId(DSlayoutId_), memoryVisability(memoryVisability_), setId(setId_), BlueprintId(BlueprintId_) {};
	DescriptorBlueprint& getBlueprint();
};
}
