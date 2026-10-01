#pragma once 
#include "StagingHeap.h"
#include "BasePage.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
struct GPULocalPage : BasePage 
{
	void init(); 	
	GPULocalPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_, uint32_t maxDormantStagingHeaps_);
};
}
