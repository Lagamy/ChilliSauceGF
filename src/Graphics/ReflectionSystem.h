#pragma once 
#include "Layout.h"
#include "Utilities.h"
#include <array>

namespace Graphics
{
struct ReflectionSystem
{
    std::array<Pool<ReflectionLayout>, BufferTypesCount> layoutsPerBufferType; 
};
} 