#pragma once
#include "MemoryEntryId.h"
#include "PoolId.h"
#include "StagingData.h"

namespace Graphics
{
using ReadbackId = PoolId;

/*
	One requested read of an entry range. For GPU_LOCAL entries stagingData is the READBACK range the copy writes into, and batchId is OpenBatchId until the copy is queued.
	CPU_SHARED entries need no copy, so their batchId is 0 and the request is ready immediately.
*/
struct ReadbackRequest
{
	MemoryEntryId memoryEntryId;
	StagingData stagingData;
	uint64_t batchId = OpenBatchId;
	uint64_t inEntryOffset = 0;
	uint64_t size = 0;
};
}
