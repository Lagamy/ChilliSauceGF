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

	DescriptorBlueprint& getBlueprint();
};
}
