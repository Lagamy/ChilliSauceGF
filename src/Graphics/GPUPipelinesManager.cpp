#include "GPUPipelinesManager.h"

namespace Graphics
{
void GPUPipelinesManager::createAllPipelines()
{
    for(auto& rGraphicsPipeline : this->graphicsPipelines.objects)
    {
        rGraphicsPipeline.create();
    }
} 

void GPUPipelinesManager::destroyAllPipelines()
{
    this->graphicsPipelines.clear(); 
    this->computePipelines.clear();
}
}