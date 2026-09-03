#include "CPUSharedPage.h"
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
		for(uint32_t i = 0; i < this->aliveFreeMemIntervals.size(); i++)
		{
			MemoryInterval& rMemoryInterval = this->freeMemIntervals.getInternal(this->aliveFreeMemIntervals[i]); 
			uint32_t intervalSize = rMemoryInterval.end - rMemoryInterval.start;
			if(size_ <= intervalSize) // suitable for this buffer 
			{
				this->heap.addOrExtendBufferDynamic(); 
				rMemoryInterval.start += size_ - 1; 
				this->heap.addOrExtendBufferDynamic(); 
				if(rMemoryInterval.start == rMemoryInterval.end)
				{
					this->freeMemIntervals.removeInternal(this->aliveFreeMemIntervals[i]);
					this->aliveFreeMemIntervals.erase(this->aliveFreeMemIntervals.begin() + i);
				}
				return; 
			}
			// If no interval was found 
			if(this->heap.memoryBlocks.back().freeSpace < size_)
			{
				this->heap.memoryBlocks.emplace_back();
			}
			this->heap.addOrExtendBufferDynamic(); 
		}
	}
};


