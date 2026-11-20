#pragma once 
#include "MemoryEntryId.h"
#include <cstdint>
#include <vector>

namespace Graphics 
{
/* One range of a GPU Local entry copy. Offsets are entry byte offsets. */
struct MemoryCopyRegion
{
    uint64_t srcOffset = 0;
    uint64_t dstOffset = 0;
    uint64_t size = 0;
};

/* A MemoryManager::copyEntry that is waiting for the source to have no upload in flight and no earlier UPDATE waiting. */
struct EntryCopyRequest 
{
    MemoryEntryId srcEntryId;
    MemoryEntryId dstEntryId;
    std::vector<MemoryCopyRegion> regions;
};
}
