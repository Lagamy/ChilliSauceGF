#pragma once 
#include "Layout.h"
#include "Pool.h"
#include "PoolMap.h"
#include "Utilities.h"
#include "VertexLayoutUpdate.h"
#include <array>

namespace Graphics
{
struct ReflectionSystem
{
    std::array<PoolMap<ReflectionLayout>, BufferTypesCount - 1> layoutsPerBufferType = {("Vertex Layouts"), ("Uniform Layouts"), ("Storage Layouts")};
    Pool<VertexLayoutUpdate> vertexLayoutUpdates = ("Vertex Layout Updates");

    PoolId beginVertexLayoutUpdate(PoolId verticeLayoutId_);
    PoolId addVariableToVerticeLayout(PoolId updateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addVariableToVerticeLayout(PoolId updateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_);
    void removeVariableFromVerticeLayout(PoolId updateId_, PoolId variableId_);
    void commitVertexLayoutUpdate(PoolId updateId_);
};
} 
