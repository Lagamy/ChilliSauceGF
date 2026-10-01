#include "BasePage.h"
#include "Api.h"
#include "MemoryBlock.h"
#include "PoolId.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics 
{
	void BasePage::setStagingDataForUpload(MemoryEntry& rEntry_)
	{
		MemoryManager& rMemoryManager = getMemoryManager();
		MemoryBlock& rMemoryBlock = this->memoryBlocks[rEntry_.memoryBlockId];

		/* 
			Every staging heap of a page is as big as one memory block. A block borrows a heap while it has pending uploads, and the heap mirrors the block: 
			an entry's data is staged at the same offset where its buffer sits inside the block. 
		*/
		if(rMemoryBlock.stagingHeapId == UninitializedPoolId)
		{
			if(this->freeStagingHeaps.empty())
			{
				rMemoryBlock.stagingHeapId = rMemoryManager.uploadStagingHeaps.add(this->memoryBlockSize);
			}
			else
			{
				rMemoryBlock.stagingHeapId = this->freeStagingHeaps.back();
				this->freeStagingHeaps.pop_back();
			}
		}

		StagingHeap& rHeap = rMemoryManager.uploadStagingHeaps[rMemoryBlock.stagingHeapId];
		rEntry_.stagingData = { rMemoryBlock.stagingHeapId, 0, rEntry_.size, this->buffersFirstByteOffset[rEntry_.bufferId] };
		rHeap.pendingUploadsCount++;
	}

	void BasePage::releaseStagingHeap(PoolId stagingHeapId_, PoolId memoryBlockId_)
	{
		this->memoryBlocks[memoryBlockId_].stagingHeapId = UninitializedPoolId;
		this->freeStagingHeaps.emplace_back(stagingHeapId_);

		/* Free heaps have no pending uploads, so nothing reads them and they can be destroyed. The oldest ones go first, since heaps are reused from the back. */
		while(this->freeStagingHeaps.size() > this->maxDormantStagingHeaps)
		{
			getMemoryManager().uploadStagingHeaps.remove(this->freeStagingHeaps.front());
			this->freeStagingHeaps.erase(this->freeStagingHeaps.begin());
		}
	}

	BufferCreationResult BasePage::addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_, const char* uploadName_)
	{
		/* 
			The real buffer is created first. Its memory requirements (size and alignment) can differ from the requested size and from the ghost buffers, 
			and vkBindBufferMemory needs the memory after the offset to hold memoryRequirements.size (VUID-vkBindBufferMemory-size-01037). 
		*/
		PoolId bufferId = this->buffers.add(size_, BufferTypeToUsage[bufferType_] | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, uploadName_); 
		VkMemoryRequirements bufferReqs = this->buffers[bufferId].memoryReqs;
		if(bufferReqs.size > this->memoryBlockSize)
		{
			this->buffers.remove(bufferId);
			throw std::runtime_error(std::format("Page: Buffer for {} needs more memory than a memory block holds.", uploadName_));
		}

		bool addedNewMemoryBlock = false;
		while(true)
		{
			/* 
				Free intervals are inclusive [start, end]. vkBindBufferMemory needs the offset to be a multiple of the buffer's alignment (VUID-vkBindBufferMemory-memoryOffset-01036), 
				so the buffer starts at the aligned start of the interval. The padding before it and the space after it stay free. 
			*/
			for(PoolId memBlockId : this->aliveMemoryBlocks)
			{
				FreeSpace& rFreeSpace = this->freeSpacePerBlock[memBlockId]; 
				for(uint32_t i = 0; i < rFreeSpace.aliveIntervals.size(); i++)
				{
					PoolId intervalId = rFreeSpace.aliveIntervals[i];
					MemoryInterval& rMemoryInterval = rFreeSpace.memoryIntervals[intervalId];
					
					uint64_t alignedStart = alignUp(rMemoryInterval.start, bufferReqs.alignment);
					if(alignedStart > rMemoryInterval.end)
					{
						continue;
					}

					VkDeviceSize intervalSize = rMemoryInterval.end - alignedStart + 1;
					if(bufferReqs.size <= intervalSize) 
					{
						uint64_t intervalStart = rMemoryInterval.start;
						uint64_t intervalEnd = rMemoryInterval.end;
						uint64_t remainderStart = alignedStart + bufferReqs.size;
						bool hasPadding = alignedStart > intervalStart;
						bool hasRemainder = remainderStart <= intervalEnd;

						rFreeSpace.intervalByFirst.erase(intervalStart); 
						rFreeSpace.intervalByLast.erase(intervalEnd);

						if(hasPadding)
						{
							rMemoryInterval.end = alignedStart - 1;
							rFreeSpace.intervalByFirst.emplace(intervalStart, intervalId);
							rFreeSpace.intervalByLast.emplace(rMemoryInterval.end, intervalId);
						}

						if(hasRemainder)
						{
							if(hasPadding)
							{
								PoolId remainderId = rFreeSpace.memoryIntervals.add(remainderStart, intervalEnd);
								rFreeSpace.aliveIntervals.emplace_back(remainderId);
								rFreeSpace.intervalByFirst.emplace(remainderStart, remainderId);
								rFreeSpace.intervalByLast.emplace(intervalEnd, remainderId);
							}
							else
							{
								rMemoryInterval.start = remainderStart;
								rFreeSpace.intervalByFirst.emplace(remainderStart, intervalId);
								rFreeSpace.intervalByLast.emplace(intervalEnd, intervalId);
							}
						}
						
						if(!hasPadding && !hasRemainder)
						{
							rFreeSpace.aliveIntervals.erase(rFreeSpace.aliveIntervals.begin() + i); 
							rFreeSpace.memoryIntervals.remove(intervalId); 
						}

						this->memoryBlocks[memBlockId].freeSpace -= bufferReqs.size;
						this->addBufferInternal(memBlockId, bufferId, alignedStart);
						return {memBlockId, bufferId};
					}
				}
			}

			if(addedNewMemoryBlock)
			{
				throw std::runtime_error(std::format("Page: Failed to place Buffer for {} into a new memory block.", uploadName_));
			}

			/* 
				No interval fits. A new memory block starts as one free interval covering all of it, and the search above is repeated, so it is the only place buffers are placed. 
				MemoryBlocks and FreeSpaces are separate Pools that are added and removed together, so a block and its FreeSpace share the same id. 
			*/
			PoolId newMemoryBlockId = this->memoryBlocks.add();
			MemoryBlock& rNewMemoryBlock = this->memoryBlocks[newMemoryBlockId];
			rNewMemoryBlock.create(this->memoryBlockSize, this->memReqs, this->memoryProperties, this->memoryVisability, this->upperBoundEntrySize, newMemoryBlockId); 
			rNewMemoryBlock.freeSpace = this->memoryBlockSize;

			PoolId freeSpaceId = this->freeSpacePerBlock.add(); 
			#ifdef ENGINE_DEBUG
				if(freeSpaceId != newMemoryBlockId)
				{
					throw std::runtime_error("Page: MemoryBlock and its FreeSpace ended up with different ids.");
				}
			#endif
			FreeSpace& rNewFreeSpace = this->freeSpacePerBlock[freeSpaceId];
			PoolId wholeBlockIntervalId = rNewFreeSpace.memoryIntervals.add(0, this->memoryBlockSize - 1);
			rNewFreeSpace.aliveIntervals.emplace_back(wholeBlockIntervalId);
			rNewFreeSpace.intervalByFirst.emplace(0, wholeBlockIntervalId);
			rNewFreeSpace.intervalByLast.emplace(this->memoryBlockSize - 1, wholeBlockIntervalId);

			this->aliveMemoryBlocks.emplace_back(newMemoryBlockId); 
			this->size += this->memoryBlockSize; 
			addedNewMemoryBlock = true;
		}
	}


	void BasePage::removeBuffer(PoolId bufferId_, PoolId memoryBlockId_)
	{
		FreeSpace& rFreeSpace = this->freeSpacePerBlock[memoryBlockId_];
		MemoryBlock& rMemoryBlock = this->memoryBlocks[memoryBlockId_];
		uint64_t firstByte = this->buffersFirstByteOffset[bufferId_]; 
		uint64_t occupiedSize = this->buffers[bufferId_].memoryReqs.size; 
		uint64_t lastByte = firstByte + occupiedSize - 1; 

		/* The space the buffer occupied becomes free and is merged with free intervals touching it, so two free intervals are never next to each other. */
		bool bordersLeft = firstByte > 0 && rFreeSpace.intervalByLast.contains(firstByte - 1); 
		bool bordersRight = rFreeSpace.intervalByFirst.contains(lastByte + 1); 

		if(bordersLeft && bordersRight)
		{
			PoolId leftId = rFreeSpace.intervalByLast.at(firstByte - 1); 
			PoolId rightId = rFreeSpace.intervalByFirst.at(lastByte + 1); 
			MemoryInterval& rLeft = rFreeSpace.memoryIntervals[leftId]; 
			MemoryInterval& rRight = rFreeSpace.memoryIntervals[rightId]; 

			rFreeSpace.intervalByLast.erase(rLeft.end);
			rFreeSpace.intervalByFirst.erase(rRight.start);
			rFreeSpace.intervalByLast.erase(rRight.end);
			rLeft.end = rRight.end; 
			rFreeSpace.intervalByLast.emplace(rLeft.end, leftId);  

			std::erase(rFreeSpace.aliveIntervals, rightId);
			rFreeSpace.memoryIntervals.remove(rightId); 
		}
		else if(bordersLeft)
		{
			PoolId leftId = rFreeSpace.intervalByLast.at(firstByte - 1); 
			MemoryInterval& rLeft = rFreeSpace.memoryIntervals[leftId]; 

			rFreeSpace.intervalByLast.erase(rLeft.end);
			rLeft.end = lastByte; 
			rFreeSpace.intervalByLast.emplace(lastByte, leftId);
		}
		else if(bordersRight)
		{
			PoolId rightId = rFreeSpace.intervalByFirst.at(lastByte + 1); 
			MemoryInterval& rRight = rFreeSpace.memoryIntervals[rightId];

			rFreeSpace.intervalByFirst.erase(rRight.start); 
			rRight.start = firstByte; 
			rFreeSpace.intervalByFirst.emplace(firstByte, rightId); 
		}
		else
		{
			PoolId intervalId = rFreeSpace.memoryIntervals.add(firstByte, lastByte); 
			rFreeSpace.aliveIntervals.emplace_back(intervalId); 
			rFreeSpace.intervalByFirst.emplace(firstByte, intervalId); 
			rFreeSpace.intervalByLast.emplace(lastByte, intervalId); 
		}

		/* The buffer is destroyed before its memory block can be freed, since a freed block must have no buffers bound to it that are still used. */
		rMemoryBlock.freeSpace += occupiedSize; 
		this->removeBufferInternal(bufferId_);
		if(rMemoryBlock.freeSpace == rMemoryBlock.size)
		{
			this->removeMemoryBlock(memoryBlockId_); 
		}
	}


	void BasePage::removeMemoryBlock(PoolId memoryBlockId_)
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

	void BasePage::addBufferInternal(PoolId memoryId_, PoolId bufferId_, VkDeviceSize bufferFirstByteOffset_)
	{
		PoolId offsetId = this->buffersFirstByteOffset.add(bufferFirstByteOffset_); 
		#ifdef ENGINE_DEBUG
			if(offsetId != bufferId_)
			{
				throw std::runtime_error("Page: Buffer and its first byte offset ended up with different ids.");
			}
		#endif
		vkBindBufferMemory(getMainDevice().logicalDevice, this->buffers[bufferId_].get(), this->memoryBlocks[memoryId_].get(), bufferFirstByteOffset_);
	}

	void BasePage::removeBufferInternal(PoolId bufferId_)
	{
		this->buffersFirstByteOffset.remove(bufferId_);
		this->buffers.remove(bufferId_); 
	}
	
	void BasePage::destroy()
	{
		this->memoryBlocks.clear(); 
		this->freeSpacePerBlock.clear(); 
		this->buffers.clear();
		this->buffersFirstByteOffset.clear();
		this->size = 0; 
	}

	
	BasePage::BasePage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_, MemoryVisabilityEnum memoryVisability_, uint32_t maxDormantStagingHeaps_) : upperBoundEntrySize(upperBoundEntrySize_), memoryBlockSize(memoryBlockSize_), memoryVisability(memoryVisability_), maxDormantStagingHeaps(maxDormantStagingHeaps_), memoryProperties(memoryVisability_ == CPU_SHARED ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}
};


