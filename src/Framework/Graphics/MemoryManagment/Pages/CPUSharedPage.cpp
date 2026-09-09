#include "CPUSharedPage.h"
#include "FreeSpace.h"
#include "PoolId.h"
#include "PoolNameless.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics 
{
	CPUSharedPage::CPUSharedPage(uint32_t upperBoundEntrySize_) : upperBoundEntrySize(upperBoundEntrySize_) {};  

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

	void CPUSharedPage::addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_)
	{
		
		// uint32_t lastMemoryBlockId = this->heap.buffersPerMemoryBlock.size() - 1;
		// std::vector<MemoryInterval>& MemIntervals = [lastMemoryBlockId];   
		for(uint32_t i = 0; i < this->freeMemoryIntervalsPerBlock.size(); i++)
		{
			MemoryInterval& rFreeMemInterval = freeMemoryIntervalsPerBlock[i]; 
			uint32_t intervalSize = rFreeMemInterval.end - rFreeMemInterval.start;
			if(size_ <= intervalSize) // suitable for this buffer 
			{
				this->heap.addBufferDynamic(); 
				this->freeByFirst.erase(rFreeMemInterval.start); 
				rFreeMemInterval.start += size_ - 1; 
				this->freeByFirst.emplace(rFreeMemInterval.start, i);
				if(rFreeMemInterval.start == rFreeMemInterval.end)
				{	
					this->freeByFirst.erase(rFreeMemInterval.start); 
					this->freeByLast.erase(rFreeMemInterval.end); 
					this->freeMemoryIntervalsPerBlock.erase(this->freeMemoryIntervalsPerBlock.begin() + i); 
				}
				return; 
			}
			// If no interval was found 
			if(this->heap.memoryBlocks.back().freeSpace < size_)
			{
				this->heap.memoryBlocks.emplace_back();
				if(this->heap.memoryBlocks.back().freeSpace != 0)
				{
					this->freeMemoryIntervalsPerBlock[this->heap.memoryBlocks.size() - 1].emplace_back(); 
					
				}
			}

			// uint32_t memoryBlockId = this->heap.memoryBlocks.size() - 1; 
			this->heap.addBufferDynamic(); 
		}
	}


	void CPUSharedPage::removeBuffer(PoolId bufferId_)
	{
		Buffer& rBuffer = this->heap.buffers[bufferId_];
		FreeSpace& rFreeSpace = this->freeSpacePerBlock[rBuffer.memoryBlockId];
		uint64_t firstByte = this->heap.bufferOffsets[bufferId_]; 
		uint64_t lastByte = firstByte + this->heap.bufferSizes[bufferId_] - 1; 
		
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
};


