#pragma once 
#include <string>
#include <vector> 
#include "Layout.h"
#include "PoolId.h"
#include "Utilities.h"

namespace Graphics
{
    struct DescriptorBlueprint 
    {
        std::string name; // for debug porpuses
        BufferTypeEnum bufferType; // Decides in which ReflectionSystem list layoutId lives.
        PoolId layoutId; // ReflectionLayout (one of the uniform / storage ones) describing the members of this descriptor.
        std::vector<PoolId> derivedDescriptors; // Ids of the DescriptorSets derived from it. The descriptor in each of them lives in the same slot as this blueprint in its DescriptorSetLayout.

        DescriptorBlueprint(const char* name_, BufferTypeEnum bufferType_, PoolId layoutId_) : name(name_), bufferType(bufferType_), layoutId(layoutId_) {};
        PoolId addMember(const char* name_, DataTypeEnum dataType_); // Note: use in editor
    };
}
