#include "DynamicAllocator.h"
#include "Buffer.h"
#include "PageInfo.h"
#include "Utilities.h"
#include <stdexcept>

namespace Graphics 
{

void DynamicAllocator::addPage(uint32_t upperBoundForEntrySize_, StorageUnitEnum upperBoundUnit_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_, bool isCpuShared_)
{
    VkDeviceSize upperBoundEntrySizeInBytes = toBytes(upperBoundForEntrySize_, upperBoundUnit_); 
    VkDeviceSize memoryBlockSizeInBytes = toBytes(memoryBlockSize_, memoryBlockSizeUnit_);

	#ifdef ENGINE_DEBUG
        if(this->initialized)
        {
            throw std::runtime_error("Dynamic Memory Allocator: Can't add Page after allocator was created.");
        }
        if(upperBoundEntrySizeInBytes > memoryBlockSizeInBytes)
        {
            throw std::runtime_error("Dynamic Memory Allocator: Can't add Page where upper bound Entry size is bigger than single memory block size.");
        }
    #endif

    if(isCpuShared_)
    {
        this->cpuSharedPageInfos.emplace_back(upperBoundEntrySizeInBytes, memoryBlockSizeInBytes); 
        return; 
    }
    this->gpuLocalPageInfos.emplace_back(upperBoundEntrySizeInBytes, memoryBlockSizeInBytes);
}

void DynamicAllocator::init()
{
    std::sort(this->cpuSharedPageInfos.begin(), this->cpuSharedPageInfos.end()); 
    std::sort(this->gpuLocalPageInfos.begin(), this->gpuLocalPageInfos.end());
    
    this->cpuSharedPages.reserve(cpuSharedPageInfos.size());
    this->gpuLocalPages.reserve(gpuLocalPageInfos.size());

    for(PageInfo info : this->cpuSharedPageInfos)
    {
        this->cpuSharedPages.emplace_back(info.upperBoundEntrySize, info.memoryBlockSize); 
    } 
    
    for(PageInfo info : this->gpuLocalPageInfos)
    {
        this->gpuLocalPages.emplace_back(info.upperBoundEntrySize, info.memoryBlockSize); 
    }

    this->initialized = true; 
}; 


UploadId DynamicAllocator::addAndUploadEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_)
{
    uint32_t id = 0; 
    
    if(memoryVisability_ == CPU_SHARED)
	{
        for(uint32_t i = 0; i < this->cpuSharedPages.size(); i++)
        {
            PageInfo& rPageInfo = this->cpuSharedPageInfos[i]; 
            if(size_ < rPageInfo.upperBoundEntrySize)
            {
                PoolId bufferId = this->cpuSharedPages[i].addBuffer(size_, uploadType_, name_); 
                Buffer& rBuffer = this->cpuSharedPages[i].buffers[bufferId]; 
   
                PoolId id = this->uploadEntries.add(name_, size_, uploadType_, memoryVisability_,i, bufferId);
                UploadEntry& rUploadEntry = this->uploadEntries[id]; 
                rUploadEntry.upload(data_, size_, 0); 

                // Upload Data
		        return {DYNAMIC, memoryVisability_, uploadType_, id}; 
            } 
        }
	}
    else 
	{
        for(uint32_t i = 0; i < this->gpuLocalPages.size(); i++)
        {
            PageInfo& rPageInfo = this->gpuLocalPageInfos[i]; 
            if(size_ < rPageInfo.upperBoundEntrySize)
            {
                PoolId bufferId = this->gpuLocalPages[i].addBuffer(size_, uploadType_, name_); 
                Buffer& rBuffer = this->gpuLocalPages[i].buffers[bufferId]; 
   
                PoolId id = this->uploadEntries.add(name_, size_, uploadType_, memoryVisability_,i, bufferId);
                UploadEntry& rUploadEntry = this->uploadEntries[id]; 
                rUploadEntry.upload(data_, size_, 0); 

                // Upload Data
		        return {DYNAMIC, memoryVisability_, uploadType_, id}; 
            } 
        }
	}
};

void removeEntry()
{

}

void DynamicAllocator::deallocate()
{
	if(this->initialized)
	{
        this->initialized = false; 
    }; 
}; 
}