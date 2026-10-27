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


	void addPage(MemoryVisabilityEnum memoryVisability_, uint32_t upperBoundForEntrySize_, StorageUnitEnum upperBoundUnit_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_);
	PoolId allocateEntry(const char* name_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	PoolId reserveEntry(const char* name_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	PoolId addAndUploadEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	void updateEntry(PoolId entryId_, const void* data_, size_t entryOffset_, size_t srcOffset_, size_t byteAmmount_); 
    void removeGPULocalEntry(PoolId entryId_);
    void removeCPUSharedEntry(PoolId entryId_);

    void init();
    void deallocate();  
};
}
