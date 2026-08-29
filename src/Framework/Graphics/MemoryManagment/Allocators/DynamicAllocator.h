#pragma once 
#include "CPUSharedPage.h"
#include "DynamicPages/GPUPage.h"
#include "GPUPage.h"
#include "StagingHeap.h"
#include "UploadEntry.h"
#include <vector>

namespace Graphics
{
struct DynamicAllocator 
{
    // Feel out before MemoryManager.setup() happens in . Its 1 time init  
    std::vector<uint32_t> cpuSharedPageSizes; 
    std::vector<uint32_t> gpuLocalPageSizes; 
    bool initialized = false; 

	std::array<std::vector<UploadEntry>, 2> uploadEntriesPerMemVisability;
    std::vector<PoolId> uploadCompletedSemaphores;  // Per entry, since upload is done per 
    
    std::vector<CPUSharedPage> cpuSharedPages; // Sorted by smallest upper bound size per entry -> biggest 
    
    StagingHeap stagingHeap; 
    std::vector<GPUPage> gpuLocalPages; // Sorted by smallest upper bound size per entry -> biggest
    
	void addPage(uint32_t upperBoundForEntrySize_, bool isCpuShared_);
    void create(); 
    DynamicAllocator(); 
};
}