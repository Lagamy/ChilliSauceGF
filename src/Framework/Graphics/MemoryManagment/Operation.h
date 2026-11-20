#pragma once 
#include "PoolId.h"

enum MemoryOperationTypeEnum 
{
    UPDATE, 
    REMOVE,
    READBACK_COPY,
    COPY_ENTRY
}; 

struct Operation 
{
    MemoryOperationTypeEnum type; 
    PoolId id; 
};