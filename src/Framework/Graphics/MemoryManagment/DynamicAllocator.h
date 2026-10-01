#pragma once 
#include "CPUSharedPage.h"
#include "GPULocalPage.h"
#include "MemoryEntry.h"
#include "MemoryEntryId.h"
#include "PageInfo.h"
#include "Utilities.h"
#include <vector>

namespace Graphics
{
struct DynamicAllocator 
{
    // Feel out before MemoryManager.setup() happens in . Its 1 time init  
    std::vector<PageInfo> cpuSharedPageInfos; 
    std::vector<PageInfo> gpuLocalPageInfos; 
    bool initialized = false; 

	Pool<MemoryEntry> memoryEntries;

    // Todo: add multithreading and make it a per thread var 
    PoolId currentEntryToUpload;
    
    std::vector<PoolId> uploadCompletedSemaphores;  // Per entry, since upload is done per 
    
    std::vector<CPUSharedPage> cpuSharedPages; // Sorted by smallest upper bound size per entry -> biggest 
    std::vector<GPULocalPage> gpuLocalPages; // Sorted by smallest upper bound size per entry -> biggest


	void addPage(MemoryVisabilityEnum memoryVisability_, uint32_t upperBoundForEntrySize_, StorageUnitEnum upperBoundUnit_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_, uint32_t maxDormantStagingHeaps_);
	PoolId addAndUploadEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	void updateEntry(PoolId entryId_, const void* data_, size_t entryOffset_, size_t srcOffset_, size_t byteAmmount_); 
    void removeGPULocalEntry(PoolId entryId_);
    void removeCPUSharedEntry(PoolId entryId_);

    void init();
    void deallocate();  
};
}

// UpdateStagingHeapId UpdateStagingPage::getStagingHeapForUse()
// {
    // if(this->freeStagingHeaps.empty())
    // {
        // PoolId id = this->updateStagingHeaps.add();
        // this->freeStagingHeaps.emplace_back(id);   
    // }
// 
    // if(this->freeStagingHeaps.size() > Globals::maxUnusedUploadStagingHeaps) // Shouldn't happen often. If it does -> your maxUnusedUpdateStagingHeaps is too low.
	// {
		// for(uint32_t i = 1; i < this->freeStagingHeaps.size(); i++)
		// {
			// this->updateStagingHeaps.remove(this->freeStagingHeaps[i]); 
            // this->freeStagingHeaps.erase(this->freeStagingHeaps.begin() + i); 
            // i--; 
		// }
	// }

    // PoolId heapId = this->freeStagingHeaps.back();
    // this->freeStagingHeaps.erase(this->freeStagingHeaps.begin() + (this->freeStagingHeaps.size() - 1)); // Remove it, since its not free anymore 
    // MemoryManager itself is responsible for adding it back it pages free array after uploading to the gpu is done.
    // return {this->pageId, heapId};
// }