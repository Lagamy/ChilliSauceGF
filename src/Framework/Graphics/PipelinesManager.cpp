#include "PipelinesManager.h"

namespace Graphics
{
void PipelinesManager::createAllPipelines()
{
    for(auto& rGraphicsPipeline : this->graphicsPipelines.objects)
    {
        rGraphicsPipeline.create();
    }
} 

void PipelinesManager::destroyAllPipelines()
{
    this->graphicsPipelines.clear(); 
    this->computePipelines.clear();
}
}