#pragma once
#include "StagingHeap.h"
#include "GPUHeap.h"
#include <vulkan/vulkan.h>

namespace Graphics
{
struct GPUPage
{
	StagingHeap stagingHeap;
	GPUHeap gpuHeap;
	uint32_t upperBoundEntrySize; 
	GPUPage(uint32_t upperBoundEntrySize_);
};
}
