#pragma once 
#include "MemoryEntryId.h"

namespace Graphics 
{
struct UpdateRequest 
{
    MemoryEntryId memoryEntryId; 
    const void* data;
    uint64_t inSrcOffset;  
    uint64_t inEntryOffset;
    uint64_t size; 
};
} 