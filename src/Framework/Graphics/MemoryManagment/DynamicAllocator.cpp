#include "DynamicAllocator.h"
#include <limits>
#include <stdexcept>

namespace Graphics 
{

DynamicAllocator::DynamicAllocator()
{
    this->cpuSharedPageSizes.emplace_back(std::numeric_limits<uint32_t>::max());
}

void DynamicAllocator::addPage(uint32_t upperBoundForEntrySize_, bool isCpuShared_)
{

	#ifdef ENGINE_DEBUG
        if(this->initialized)
        {
            throw std::runtime_error("Dynamic Memory Allocator: Can't add Page after allocator was created.");
        }
    #endif

    if(isCpuShared_)
    {
        this->cpuSharedPages.emplace_back(upperBoundForEntrySize_); 
        return; 
    }
    this->gpuLocalPageSizes.emplace_back(upperBoundForEntrySize_);
}

void DynamicAllocator::create()
{
    std::sort(this->cpuSharedPageSizes.begin(), this->cpuSharedPageSizes.end()); 
    std::sort(this->gpuLocalPageSizes.begin(), this->gpuLocalPageSizes.end());
    this->cpuSharedPages.reserve(cpuSharedPageSizes.size());
    this->gpuLocalPages.reserve(gpuLocalPageSizes.size());

    for(uint32_t size : this->cpuSharedPageSizes)
    {
        this->cpuSharedPages.emplace_back(size); 
    } 
    
    for(uint32_t size : this->gpuLocalPageSizes)
    {
        this->gpuLocalPages.emplace_back(size); 
    }

    this->initialized = true; 
}; 


UploadId DynamicAllocator::addEntryAndUpload(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_)
{
    uint32_t id = 0; 
    if(memoryVisability_ == GPU_ONLY)
	{
        
        for(uint32_t pageSize : this->cpuSharedPageSizes)
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