#pragma once 
#include "PoolId.h"

enum MemoryOperationTypeEnum 
{
    UPLOAD, // For Dynamic Allocator only. 
    UPDATE, 
    REMOVE 
}; 

struct Operation 
{
    MemoryOperationTypeEnum type; 
    PoolId id; 
};