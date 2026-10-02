#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <cstdint>
#include <limits>

struct StagingData
{
    PoolId heapId = UninitializedPoolId;
    uint64_t inEntryOffset = 0;
    uint64_t size = 0;

    // Those one needed mainly for dynamic upload.
    uint64_t stagingOffset = 0;
};