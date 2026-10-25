#include "StaticAllocator.h"
#include "Api.h"
#include "Utilities.h"

namespace Graphics
{
PoolId StaticAllocator::addEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_) 
{
	
	#ifdef ENGINE_DEBUG
		if(this->allocated)
		{
			throw std::runtime_error("Static Allocator: Can't add entry to already allocated Static heaps.");  
		}
	#endif
	
	if(memoryVisability_ == GPU_LOCAL)
	{
		this->memoryEntries.emplace_back(name_, data_, size_, uploadType_, GPU_LOCAL, this->gpuLocalHeap.bufferSizes[uploadType_]); 
		this->memoryEntryIdPerBuffTypePerMemVisability[memoryVisability_][uploadType_].emplace_back(this->memoryEntries.size() - 1); 
		this->gpuLocalHeap.bufferSizes[uploadType_] += size_; 
		return PoolId(this->memoryEntries.size() - 1, 0);
	}
	else 
	{
		this->memoryEntries.emplace_back(name_, data_, size_, uploadType_, CPU_SHARED, this->cpuSharedHeap.bufferSizes[uploadType_]);
		this->memoryEntryIdPerBuffTypePerMemVisability[memoryVisability_][uploadType_].emplace_back(this->memoryEntries.size() - 1); 
		this->cpuSharedHeap.bufferSizes[uploadType_] += size_; 
		this->cpuSharedHeap.size += size_; 
		return PoolId(this->memoryEntries.size() - 1, 0);
	}
}


MemoryBlock& StaticAllocator::getGPUMemoryBlock()
{
	return this->gpuLocalHeap.memoryBlock; 
}

Buffer& StaticAllocator::getBuffer(BufferTypeEnum uploadType_, MemoryVisabilityEnum memoryVisability_)
{
	if(memoryVisability_ == CPU_SHARED)
	{
		return this->cpuSharedHeap.buffers[uploadType_];
	}
	return this->gpuLocalHeap.buffers[uploadType_];
}

/*
	GPU_LOCAL entries are staged like any other upload: each one gets its own range from the StagingAllocator. The range stays open until upload() queues the copy and tags it with that batch.
	It is only reused after that batch's fence was observed signaled.
	CPU_SHARED entries are written straight into the mapped heap memory.
*/
void StaticAllocator::allocateAndUpload()
{
	this->cpuSharedHeap.create(CPU_SHARED);
	this->gpuLocalHeap.create(GPU_LOCAL);
	this->allocated = true;

	MemoryManager& rMemoryManager = getMemoryManager();
	for(uint32_t i = 0; i < BufferTypesCount; i++)
	{
		for(uint32_t entryId : this->memoryEntryIdPerBuffTypePerMemVisability[CPU_SHARED][i])
		{
			MemoryEntry& rEntry = this->memoryEntries[entryId];
			rEntry.inGPUFirstByte = this->cpuSharedHeap.bufferOffsets[i] + rEntry.inBufferFirstByte;
			rEntry.upload(rEntry.data, 0, 0, rEntry.size);
		}

		for(uint32_t entryId : this->memoryEntryIdPerBuffTypePerMemVisability[GPU_LOCAL][i])
		{
			MemoryEntry& rEntry = this->memoryEntries[entryId];
			rEntry.inGPUFirstByte = this->gpuLocalHeap.bufferFirstByte[i] + rEntry.inBufferFirstByte;
			rEntry.stagingData = rMemoryManager.stagingAllocator.allocate(UPLOAD, rEntry.size);
			rEntry.upload(rEntry.data, 0, 0, rEntry.size);
		}
	}
}

void StaticAllocator::deallocate()
{
	if(this->allocated)
	{
		this->gpuLocalHeap.destroy(); 
		this->cpuSharedHeap.destroy(); 
		this->allocated = false; 
	}
}
}
