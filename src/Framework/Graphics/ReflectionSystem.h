#pragma once 
#include "Layout.h"
#include "PoolMap.h"
#include "Utilities.h"
#include <array>

namespace Graphics
{
struct ReflectionSystem
{
    std::array<PoolMap<ReflectionLayout>, BufferTypesCount - 1> layoutsPerBufferType = {("Vertex Layouts"), ("Uniform Layouts"), ("Storage Layouts")};
};
} 