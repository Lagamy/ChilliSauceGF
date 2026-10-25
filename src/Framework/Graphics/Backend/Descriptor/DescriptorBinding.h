#pragma ocnce 
#include <cstdint>
#include "PoolId.h"

struct DescriptorBinding
{
    PoolId setLayoutId;
    uint32_t firstDescriptorInSetLayoutId;
    uint32_t lastDescriptorInSetLayoutId;
};