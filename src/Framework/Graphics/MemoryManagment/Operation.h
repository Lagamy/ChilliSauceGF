#pragma once 
#include "PoolId.h"

enum MemoryOperationTypeEnum 
{
    UPDATE, 
    REMOVE,
    READBACK_COPY
}; 

struct Operation 
{
    MemoryOperationTypeEnum type; 
    PoolId id; 
};