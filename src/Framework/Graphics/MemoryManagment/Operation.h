#pragma once 
#include "PoolId.h"

enum MemoryOperationTypeEnum 
{
    UPDATE, 
    REMOVE 
}; 

struct Operation 
{
    MemoryOperationTypeEnum type; 
    PoolId id; 
};