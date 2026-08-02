#pragma once 
#include "Pool.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

namespace Graphics
{
struct PipelinesManager
{
	Pool<GraphicsPipeline> graphicsPipelines = ("Graphics Pipelines");
    Pool<ComputePipeline> computePipelines = ("Compute Pipelines");

    void createAllPipelines(); 
    void destroyAllPipelines();
}; 
}