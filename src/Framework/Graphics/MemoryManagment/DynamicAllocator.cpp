#include "DynamicAllocator.h"
#include "Api.h"
#include "BasePage.h"
#include "Buffer.h"
#include "CPUSharedPage.h"
#include "GPULocalPage.h"
#include "PageInfo.h"
#include "Utilities.h"
#include <stdexcept>

namespace Graphics 
{

void DynamicAllocator::addPage(MemoryVisabilityEnum memoryVisability_, uint32_t upperBoundForEntrySize_, StorageUnitEnum upperBoundUnit_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_)
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

    if(memoryVisability_ == CPU_SHARED)
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
        this->cpuSharedPages.emplace_back(info.upperBoundEntrySize, info.memoryBlockSize).init();
    }
    
    for(PageInfo info : this->gpuLocalPageInfos)
    {
        this->gpuLocalPages.emplace_back(info.upperBoundEntrySize, info.memoryBlockSize).init();
    }

    this->initialized = true; 
}; 


/*
	Creates the buffer and the entry but writes no data. A GPU_LOCAL entry also gets a whole-entry staging range, which stays open until the entry's copy is queued (MemoryEntry::queueStagedCopy tags it with the batch).
*/
PoolId DynamicAllocator::reserveEntry(const char* name_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_)
{
    if(memoryVisability_ == CPU_SHARED)
	{
        if(size_ > this->cpuSharedPageInfos.back().upperBoundEntrySize)
        {
            throw std::runtime_error(std::format("Dynamic Allocator: unable to find page with upper bound big enough for {}.", name_));
        }
        
        for(uint32_t i = 0; i < this->cpuSharedPages.size(); i++)
        {
            PageInfo& rPageInfo = this->cpuSharedPageInfos[i]; 
            if(size_ <= rPageInfo.upperBoundEntrySize)
            {
                BufferCreationResult bufferCreationResult = this->cpuSharedPages[i].addBuffer(size_, uploadType_, name_);
                return this->memoryEntries.add(name_, size_, uploadType_, memoryVisability_,i, bufferCreationResult.memoryBlockId, bufferCreationResult.bufferId);
            } 
        }
	}
    else 
	{
        if(size_ > this->gpuLocalPageInfos.back().upperBoundEntrySize)
        {
            throw std::runtime_error(std::format("Dynamic Allocator: unable to find page with upper bound big enough for {}.", name_));
        }
        
        for(uint32_t i = 0; i < this->gpuLocalPages.size(); i++)
        {
            PageInfo& rPageInfo = this->gpuLocalPageInfos[i]; 
            if(size_ <= rPageInfo.upperBoundEntrySize)
            {
                BufferCreationResult bufferCreationResult = this->gpuLocalPages[i].addBuffer(size_, uploadType_, name_);

                PoolId id = this->memoryEntries.add(name_, size_, uploadType_, memoryVisability_,i, bufferCreationResult.memoryBlockId, bufferCreationResult.bufferId);
                this->memoryEntries[id].stagingData = getMemoryManager().stagingAllocator.allocate(UPLOAD, size_);
                return id;
            } 
        }
	}
    throw std::runtime_error("How did you get there?");
}

PoolId DynamicAllocator::addAndUploadEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_)
{
    PoolId id = this->reserveEntry(name_, size_, memoryVisability_, uploadType_);
    this->memoryEntries[id].upload(data_, 0, 0, size_);
    return id;
}


void DynamicAllocator::removeGPULocalEntry(PoolId entryId_)
{
    MemoryEntry& rEntry = this->memoryEntries[entryId_]; 
    GPULocalPage& rPage = this->gpuLocalPages[rEntry.pageId];
    rPage.removeBuffer(rEntry.bufferId, rEntry.memoryBlockId); 
}

void DynamicAllocator::removeCPUSharedEntry(PoolId entryId_)
{
    MemoryEntry& rEntry = this->memoryEntries[entryId_]; 
    CPUSharedPage& rPage = this->cpuSharedPages[rEntry.pageId];
    rPage.removeBuffer(rEntry.bufferId, rEntry.memoryBlockId); 
}

void DynamicAllocator::deallocate()
{
	if(this->initialized)
	{
        /* Pages own Pools of MemoryBlock and Buffer, which have no destructors, so each page must be destroyed explicitly. */
        for(CPUSharedPage& rPage : this->cpuSharedPages)
        {
            rPage.destroy();
        }
        for(GPULocalPage& rPage : this->gpuLocalPages)
        {
            rPage.destroy();
        }
        this->initialized = false;
    }; 
}; 
}