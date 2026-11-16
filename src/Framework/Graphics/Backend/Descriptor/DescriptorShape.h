#pragma once 
#include <vector>
#include "RepeatShape.h"

namespace Graphics
{
    /* Where a member sits in the block. shape holds the layers of a STRUCT_MEMBER (strides relative to its first byte), a variable has none. */
    struct MemberPlacement
    {
        uint64_t firstByteId = 0;
        RepeatShape shape;
    };

    /*
        The bytes of one descriptor: dynamicLayer.count units of one block, each block holding the members one after another.
        Built in DescriptorLayout::makeShape. A descriptor that isn't dynamic is a single block (dynamicLayer.count 0).
    */
    struct DescriptorShape
    {
        std::vector<MemberPlacement> placements; // indexed by the slot of the member in DescriptorLayout::members
        uint64_t blockSize = 0; // one unit, without the padding to the dynamic offset alignment
        RepeatLayer dynamicLayer = {0, 0}; // count 0 = not dynamic. stride is the block size padded to the dynamic offset alignment

        bool isDynamic() const
        {
            return this->dynamicLayer.count > 0;
        }

        /* The whole entry. */
        uint64_t getSize() const
        {
            return this->isDynamic() ? this->dynamicLayer.count * this->dynamicLayer.stride : this->blockSize;
        }
    };
}
