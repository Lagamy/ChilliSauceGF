#pragma once 
#include "MemoryEntryId.h"
#include "StagingData.h"

namespace Graphics 
{
/* A GPU_LOCAL update whose bytes are already written into stagingData's range. Only the copy is waiting for the entry to have no upload in flight. */
struct UpdateRequest 
{
    MemoryEntryId memoryEntryId; 
    StagingData stagingData;
};
} 