#include "DynamicAllocator.h"
#include <limits>
#include <stdexcept>

namespace Graphics 
{

DynamicAllocator::DynamicAllocator()
{
    this->cpuSharedPageInfos.emplace_back(std::numeric_limits<uint32_t>::max());
}

void DynamicAllocator::addPage(uint32_t upperBoundForEntrySize_, uint32_t memoryBlockSize_, bool isCpuShared_)
{

	#ifdef ENGINE_DEBUG
        if(this->initialized)
        {
            throw std::runtime_error("Dynamic Memory Allocator: Can't add Page after allocator was created.");
        }
    #endif

    if(isCpuShared_)
    {
        this->cpuSharedPageInfos.emplace_back(upperBoundForEntrySize_, memoryBlockSize_); 
        return; 
    }
    this->gpuLocalPageInfos.emplace_back(upperBoundForEntrySize_, memoryBlockSize_);
}

void DynamicAllocator::create()
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


UploadId DynamicAllocator::addEntryAndUpload(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_)
{
    uint32_t id = 0; 
    if(memoryVisability_ == GPU_ONLY)
	{
        
        for(uint32_t pageSize : this->cpuSharedPageInfos)
        {
            if(size_ < pageSize)
            {
                GpuHeap& 
		        this->uploadEntriesPerMemVisability[memoryVisability_].emplace_back(name_, data_, size_, uploadType_, this->gpuHeap.bufferSizes[uploadType_]);
                this->gpuHeap.bufferSizes[uploadType_] += size_; 
		        this->stagingHeap.size += size_; 
		        return {STATIC, memoryVisability_, uploadType_, this->uploadEntryGroupPerMemVisability[memoryVisability_][uploadType_].size() - 1};
                break; 
            }
        }
	}
	else 
	{
		this->uploadEntryGroupPerMemVisability[memoryVisability_][uploadType_].emplace_back(name_, data_, size_, uploadType_);
		this->cpuSharedHeap.bufferSizes[uploadType_] += size_; 
		this->cpuSharedHeap.size += size_; 
		return {STATIC, memoryVisability_, uploadType_, this->uploadEntryGroupPerMemVisability[memoryVisability_][uploadType_].size() - 1};
	}
};

void DynamicAllocator::deallocate()
{
	if(this->initialized)
	{
        this->initialized = false; 
    }; 
}; 
}