#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <cstdint>
#include <limits>

/* Batch id of a staging range whose copy is not queued yet. Such a range is never freed, queueCopy replaces this with the real batch id (StagingAllocator::tagBatch). */
inline const uint64_t OpenBatchId = std::numeric_limits<uint64_t>::max();

struct StagingData
{
    PoolId heapId = UninitializedPoolId;
    uint64_t inEntryOffset = 0;
    uint64_t size = 0;

    uint64_t stagingOffset = 0;

    // Page of StagingAllocator that owns heapId, inside the pages of this direction.
    uint32_t stagingPageId = UninitializedId;
    Graphics::StagingDirectionEnum direction = Graphics::UPLOAD;
};