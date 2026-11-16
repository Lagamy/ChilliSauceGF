#pragma once
#include "PoolMap.h"
#include <cstring>
#include <vector>

struct CPUDataContainer 
{
    std::vector<char> data; 
    
    uint32_t repeatCount = 1;
    /* Bytes between repeat units, 0 = the layout's size (packed). A dynamic uniform's units are padded to the offset alignment. */
    uint64_t repeatStride = 0;
    /* Added to every repeat unit offset: the byte offset of a multi layer element (RepeatShape::getOffset), written with unit id 0. */
    uint64_t repeatBaseOffset = 0;

    void write(uint64_t offset_, const void* pSrc_, uint64_t size_)
    {
        std::memcpy(&this->data[offset_], pSrc_, size_);
    }

    void read(uint64_t offset_, void* pDst_, uint64_t size_) const
    {
        std::memcpy(pDst_, &this->data[offset_], size_);
    }
}; 
