#pragma once 
#include "Utilities.h"

namespace Graphics
{
struct PassId 
{
    CmdLifetimeEnum cmdLifetime; 
    QueueFamilyEnum queueFamily; 
    size_t id; 
};
} 