#pragma once 
#include <vector> 
#include "Layout.h"
#include "PoolId.h"

namespace Graphics
{
    struct DescriptorBlueprint 
    {
        ReflectionLayout layout;
        std::vector<PoolId> derivedDescriptors;
    };
}