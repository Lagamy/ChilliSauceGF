#pragma once
#include "Utilities.h"
#include "PoolId.h"

namespace Graphics
{
struct SubmissionBatchId
{
    PoolId poolId; 
    QueueFamilyEnum queueFamily; 
};
}