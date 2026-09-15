#pragma once 
#include "CPUSharedPage.h"
#include "GPULocalPage.h"
#include "StagingHeap.h"
#include "UploadEntry.h"
#include "UploadId.h"
#include "PageInfo.h"
#include <vector>

namespace Graphics
{
struct DynamicAllocator 
{
    // Feel out before MemoryManager.setup() happens in . Its 1 time init  
    std::vector<PageInfo> cpuSharedPageInfos; 
    std::vector<PageInfo> gpuLocalPageInfos; 
    bool initialized = false; 

	std::array<std::vector<UploadEntry>, 2> uploadEntriesPerMemVisability;
    std::vector<PoolId> uploadCompletedSemaphores;  // Per entry, since upload is done per 
    
    std::vector<CPUSharedPage> cpuSharedPages; // Sorted by smallest upper bound size per entry -> biggest 
    
    std::vector<GPULocalPage> gpuLocalPages; // Sorted by smallest upper bound size per entry -> biggest
    uint32_t uploadCooldownMs;  

	void addPage(uint32_t upperBoundForEntrySize_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_, bool isCpuShared_);
	UploadId addEntryAndUpload(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	void updateEntry(UploadId entryId_, const void* data_, size_t entryOffset_, size_t srcOffset_, size_t byteAmmount_); 
   
    void uploadScheduledEntries(); // Uploads every entry from uploadEntriesPerMemVisability. and clears both vectors. 
    void create();
    void deallocate();  
    DynamicAllocator(); 
};
}