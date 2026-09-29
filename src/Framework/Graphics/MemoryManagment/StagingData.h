#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <cstdint>
#include <limits>

struct StagingData
{
    PoolId heapId = UninitializedPoolId;
    uint64_t inEntryOffset;
    uint64_t size;

    // Those one needed mainly for dynamic upload.
    uint64_t stagingOffset = 0;
};