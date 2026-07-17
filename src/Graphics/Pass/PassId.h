#pragma once 
#include "Utilities.h"

namespace Graphics
{
struct PassId 
{
    CmdTypeEnum cmdType; 
    QueueFamilyEnum queueFamily; 
    size_t id; 
};
} 