#pragma once
#include "PendingCopy.h"
#include "PassId.h"
#include "PoolId.h"
#include <vector>

namespace Graphics
{
/*
	One upload batch slot: its own fence, ONESHOT TRANSFER pass and command buffer, so a batch can be in flight while the next one is filled.
	The slot carries a batch id from its first queued copy until its fence was observed signaled (queuedBatchId while filling, recordedBatchId once started). 0 means none.
*/
struct UploadBatch
{
	PoolId fenceId;
	PassId passId;
	uint32_t cmdId = 0;
	std::vector<PendingCopy> queuedCopies;
	std::vector<PendingCopy> recordedCopies;
	uint64_t queuedBatchId = 0;
	uint64_t recordedBatchId = 0;
	bool inFlight = false;
};
}
