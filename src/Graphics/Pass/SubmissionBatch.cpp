#include "SubmissionBatch.h"

namespace Graphics 
{
    SubmissionBatch::SubmissionBatch(std::string& name_, bool oneShot_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_) : name(name_), oneShot(oneShot_), queueFamily(queueFamily_), signalFenceId(signalFenceId_), dynamicSignalFence(false) {};
    SubmissionBatch::SubmissionBatch(std::string& name_, bool oneShot_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc signalRetrivalFunc_) : name(name_), oneShot(oneShot_), queueFamily(queueFamily_), dynamicSignalFenceFunc(signalRetrivalFunc_), dynamicSignalFence(true) {};
}