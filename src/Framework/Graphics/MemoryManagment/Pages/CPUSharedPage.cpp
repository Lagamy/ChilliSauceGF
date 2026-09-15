#include "CPUSharedPage.h"
#include "Api.h"
#include "MemoryBlock.h"
#include "Utilities.h"

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
	
	void CPUSharedPage::addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_, const char* uploadName_)
	{
		for(PoolId memBlockId : this->aliveMemoryBlocks)
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
					this->addBufferInternal(memBlockId, rMemoryInterval.start, size_, bufferType_, uploadName_);
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
		PoolId lastMemoryBlockId = this->aliveMemoryBlocks.back(); 
		MemoryBlock& rLastMemoryBlock = this->memoryBlocks[lastMemoryBlockId];  
		uint64_t memoryOffset = rLastMemoryBlock.size - rLastMemoryBlock.freeSpace - 1; 
		if(rLastMemoryBlock.freeSpace < size_)
		{
			if(this->memoryBlocks.back().freeSpace != 0) // Free space Interval was formed, since there was still free bytes im memoryBlock 
			{
				PoolId id = this->freeSpacePerBlock[lastMemoryBlockId].memoryIntervals.add(rLastMemoryBlock.size - rLastMemoryBlock.freeSpace - 1, rLastMemoryBlock.size); 
				this->freeSpacePerBlock[lastMemoryBlockId].aliveIntervals.emplace_back(id);
			}
				
			lastMemoryBlockId = this->memoryBlocks.add(this->memoryBlockSize, this->memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uploadName_);
			this->freeSpacePerBlock.add(); 
			this->aliveMemoryBlocks.emplace_back(lastMemoryBlockId); 
			lastMemoryBlockId = this->aliveMemoryBlocks.back(); 
			this->size += this->memoryBlockSize; 
			memoryOffset = 0;
		}
		rLastMemoryBlock = this->memoryBlocks[lastMemoryBlockId];
		rLastMemoryBlock.freeSpace -= size_;
		this->addBufferInternal(lastMemoryBlockId, memoryOffset,  size_, bufferType_, uploadName_);
	}


	void CPUSharedPage::removeBuffer(PoolId bufferId_)
	{
		Buffer& rBuffer = this->buffers[bufferId_];
		FreeSpace& rFreeSpace = this->freeSpacePerBlock[rBuffer.memoryBlockId];
		MemoryBlock& rMemoryBlock = this->memoryBlocks[rBuffer.memoryBlockId];
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
				rMemoryBlock.freeSpace += this->bufferSizes[bufferId_]; 
				if(rMemoryBlock.freeSpace == rMemoryBlock.size)
				{
					this->removeMemoryBlock(rBuffer.memoryBlockId); 
				}
				this->removeBufferInternal(bufferId_);
				return;
			}
			
			// Else left absorbes just buffer's space   
			rMemIntervalToLeft.end = lastByte; 
			rFreeSpace.intervalByLast.emplace(lastByte, memIntervalToLeftId);
			rMemoryBlock.freeSpace += this->bufferSizes[bufferId_]; 
			if(rMemoryBlock.freeSpace == rMemoryBlock.size)
			{
				this->removeMemoryBlock(rBuffer.memoryBlockId); 
			}
			this->removeBufferInternal(bufferId_); 
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


	void CPUSharedPage::removeMemoryBlock(PoolId memoryBlockId_)
	{
		for(uint32_t i = 0; i < this->aliveMemoryBlocks.size(); i++) // Expensive as hell. 
		{
			if(this->aliveMemoryBlocks[i] == memoryBlockId_)
			{
				this->aliveMemoryBlocks.erase(this->aliveMemoryBlocks.begin() + i);
				i--;  
			}
		}
		this->freeSpacePerBlock.remove(memoryBlockId_); 
		this->memoryBlocks.remove(memoryBlockId_); 
	} 

	PoolId CPUSharedPage::addBufferInternal(PoolId memoryId_, VkDeviceSize memoryOffset_, VkDeviceSize size_, BufferTypeEnum bufferType_, const char* uploadName_)
	{
		PoolId id = this->bufferOffsets.add(memoryOffset_); 
		this->bufferSizes.add(size_);
		this->buffers.add(size_, BufferTypeToUsage[bufferType_], VK_SHARING_MODE_EXCLUSIVE, memoryId_, uploadName_); 
		vkBindBufferMemory(getMainDevice().logicalDevice, this->buffers[id].get(), this->memoryBlocks[memoryId_].get(), memoryOffset_);
		return id;
	}

	void CPUSharedPage::removeBufferInternal(PoolId bufferId_)
	{
		this->bufferOffsets.remove(bufferId_);
		this->bufferSizes.remove(bufferId_); 
		this->buffers.remove(bufferId_); 
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


