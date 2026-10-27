#pragma once 
#include <vector> 
#include "Layout.h"
#include "PoolId.h"

namespace Graphics
{
    struct DescriptorBlueprint 
    {
        PoolId layoutId;
        std::vector<PoolId> derivedDescriptors;
        
        void addMember(); // Note: use in editor
    };
}