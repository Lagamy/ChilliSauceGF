#pragma once
#include "PoolMap.h"
#include <cstring>
#include <vector>

struct CPUDataContainer 
{
    std::vector<char> data; 
    uint32_t repeatCount = 1;

    void write(uint64_t offset_, const void* pSrc_, uint64_t size_)
    {
        std::memcpy(&this->data[offset_], pSrc_, size_);
    }

    void read(uint64_t offset_, void* pDst_, uint64_t size_) const
    {
        std::memcpy(pDst_, &this->data[offset_], size_);
    }
}; 
