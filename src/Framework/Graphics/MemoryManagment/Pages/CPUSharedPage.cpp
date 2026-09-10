#include "CPUSharedPage.h"
#include "FreeSpace.h"
#include "PoolId.h"
#include "PoolNameless.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics 
{
	void CPUSharedPage::init()
	{
		std::array<Buffer, BufferTypesCount> ghostBuffers; 
		ghostBuffers[0].create(1,VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Index memType inclusion in memReqs"); 
		ghostBuffers[1].create(1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Vertex memType inclusion in memReqs"); 
		ghostBuffers[2].create(1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Uniform memType inclusion in memReqs"); 
		ghostBuffers[3].create(1,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Storage memType inclusion in memReqs"); 
	
		for(uint8_t i = 0; i < BufferTypesCount; i++)
		{
			memReqs[i] = ghostBuffers[i].memoryReqs;
		}
		// ghost buffers destructors are called automatically by array  
	}
	
	PoolId addBufferInternal(uint32_t memoryId_, VkDeviceSize size_, BufferTypeEnum bufferType_)
	{

	}

	void removeBufferInternal(uint32_t memoryId_, PoolId bufferId_)
	{

	}
	
	void CPUSharedPage::addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_)
	{
		
		for (uint32_t memBlockId = 0; memBlockId < this->freeSpacePerBlock.size(); memBlockId++)
		// for(FreeSpace& rFreeSpace : this->freeSpacePerBlock)
		{
			FreeSpace& rFreeSpace = this->freeSpacePerBlock[memBlockId]; 
			for(uint32_t i = 0; i < rFreeSpace.aliveIntervals.size(); i++)
			{
				PoolId intervalId = rFreeSpace.aliveIntervals[i];
				MemoryInterval& rMemoryInterval = rFreeSpace.memoryIntervals[intervalId];  
				uint32_t intervalSize = rMemoryInterval.end - rMemoryInterval.start;
				if(size_ <= intervalSize) // suitable for this buffer 
				{

					this->addBufferInternal(memBlockId, size_, bufferType_);
					rFreeSpace.intervalByFirst.erase(rMemoryInterval.start); 
					rMemoryInterval.start += size_ - 1; 
					rFreeSpace.intervalByFirst.emplace(rMemoryInterval.start, intervalId);

					if(rMemoryInterval.start == rMemoryInterval.end)
					{	
						rFreeSpace.intervalByFirst.erase(rMemoryInterval.start); 
						rFreeSpace.intervalByLast.erase(rMemoryInterval.end); 
						rFreeSpace.aliveIntervals.erase(rFreeSpace.aliveIntervals.begin() + i); 
						rFreeSpace.memoryIntervals.remove(intervalId); 
					}
					return; 
				}
			}
		}

		// If no interval was found 
		uint32_t lastMemoryBlockId = this->memoryBlocks.size() - 1; 
		MemoryBlock& rLastMemoryBlock = this->memoryBlocks.back();  
		if(this->memoryBlocks.back().freeSpace < size_)
		{
			if(this->memoryBlocks.back().freeSpace != 0) // Free space Interval was formed, since there was still free bytes im memoryBlock 
			{
				PoolId id = this->freeSpacePerBlock[lastMemoryBlockId].memoryIntervals.add(rLastMemoryBlock.size - rLastMemoryBlock.freeSpace - 1, rLastMemoryBlock.size); 
				this->freeSpacePerBlock[lastMemoryBlockId].aliveIntervals.emplace_back(id);
			}
				
			this->memoryBlocks.emplace_back(this->memoryBlockSize, this->memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Dynamic Memory Block");
			this->freeSpacePerBlock.emplace_back(); 
			this->size += this->memoryBlockSize; 
			lastMemoryBlockId++; 
		}
		this->addBufferInternal(lastMemoryBlockId, size_, bufferType_);
	}


	void CPUSharedPage::removeBuffer(PoolId bufferId_)
	{
		Buffer& rBuffer = this->buffers[bufferId_];
		FreeSpace& rFreeSpace = this->freeSpacePerBlock[rBuffer.memoryBlockId];
		uint64_t firstByte = this->bufferOffsets[bufferId_]; 
		uint64_t lastByte = firstByte + this->bufferSizes[bufferId_] - 1; 
		
		// Check if borders free mem interval to the left 
		if(rFreeSpace.intervalByLast.contains(firstByte - 1)) // 
		{
			PoolId memIntervalToLeftId = rFreeSpace.intervalByLast.at(firstByte - 1); 
			MemoryInterval& rMemIntervalToLeft = rFreeSpace.memoryIntervals[memIntervalToLeftId]; 
			rFreeSpace.intervalByLast.erase(rMemIntervalToLeft.end);

			// Left absorbes both buffer's space and interval to the right 
			if(rFreeSpace.intervalByFirst.contains(lastByte + 1)) 
			{
				PoolId memIntervalToRightId = rFreeSpace.intervalByFirst.at(lastByte + 1); 
				MemoryInterval& rMemIntervalToRight = rFreeSpace.memoryIntervals[memIntervalToRightId]; 
				rMemIntervalToLeft.end = rMemIntervalToRight.end; 
				rFreeSpace.intervalByLast.emplace(rMemIntervalToLeft.end, memIntervalToLeftId);  
				
				rFreeSpace.intervalByLast.erase(rMemIntervalToRight.end);
				rFreeSpace.intervalByFirst.erase(rMemIntervalToRight.start);
				for(uint32_t i = 0; i < rFreeSpace.aliveIntervals.size(); i++) // expensive part 
				{
					if(rFreeSpace.aliveIntervals[i] == memIntervalToRightId)
					{
						rFreeSpace.aliveIntervals.erase(rFreeSpace.aliveIntervals.begin() + i); 
						break; 
					}		
				} 

				rFreeSpace.memoryIntervals.remove(memIntervalToRightId); 
				
				return;
			}
			
			// Else left absorbes just buffer's space   
			rMemIntervalToLeft.end = lastByte; 
			rFreeSpace.intervalByLast.emplace(lastByte, memIntervalToLeftId);  
			return; 
		}

		// Else check if it borders free mem interval to the right 
		if(rFreeSpace.intervalByFirst.contains(lastByte + 1)) 
		{
			PoolId memIntervalToRightId = rFreeSpace.intervalByFirst.at(lastByte + 1); 
			MemoryInterval& rMemIntervalToRight = rFreeSpace.memoryIntervals[memIntervalToRightId];
			rFreeSpace.intervalByFirst.erase(rMemIntervalToRight.start); 
			rMemIntervalToRight.start = firstByte; 
			rFreeSpace.intervalByFirst.emplace(firstByte, memIntervalToRightId); 
		}
		// Else -> create new free memory interval 
		PoolId id = rFreeSpace.memoryIntervals.add(firstByte, lastByte); 
		rFreeSpace.aliveIntervals.emplace_back(id); 
	}

	PoolId CPUSharedPage::addBufferInternal(uint32_t memoryId_, VkDeviceSize size_, BufferTypeEnum bufferType_)
	{

	}

	void CPUSharedPage::removeBufferInternal(uint32_t memoryId_, PoolId bufferId_)
	{

	}
	
	void CPUSharedPage::destroy()
	{
		this->memoryBlocks.clear(); 
		this->freeSpacePerBlock.clear(); 
		this->buffers.clear();
		this->bufferSizes.clear(); 
		this->bufferOffsets.clear();
		this->size = 0; 
	}

	
	CPUSharedPage::CPUSharedPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_) : upperBoundEntrySize(upperBoundEntrySize_), memoryBlockSize(memoryBlockSize_){}
};


