#pragma once 
#include "PoolMap.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

namespace Graphics
{
struct PipelinesManager
{
	PoolMap<GraphicsPipeline> graphicsPipelines = ("Graphics Pipelines");
    PoolMap<ComputePipeline> computePipelines = ("Compute Pipelines");

    void createAllPipelines(); 
    void destroyAllPipelines();
}; 
}