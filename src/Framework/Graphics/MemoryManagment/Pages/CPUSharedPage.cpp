#include "CPUSharedPage.h"

namespace Graphics 
{

	void CPUSharedPage::init()
	{
		std::array<Buffer, BufferTypesCount> ghostBuffers; 
		ghostBuffers[0].create("Ghost buffer for Index memType inclusion in memReqs", 1,VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE); 
		ghostBuffers[1].create("Ghost buffer for Vertex memType inclusion in memReqs", 1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE); 
		ghostBuffers[2].create("Ghost buffer for Uniform memType inclusion in memReqs", 1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE); 
		ghostBuffers[3].create("Ghost buffer for Storage memType inclusion in memReqs", 1,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE); 
	
		for(uint8_t i = 0; i < BufferTypesCount; i++)
		{
			memReqs[i] = ghostBuffers[i].memoryReqs;
		}

		/* Ghost buffers only exist to read memoryReqs. Buffer has no destructor, so they must be destroyed here. */
		for(uint8_t i = 0; i < BufferTypesCount; i++)
		{
			ghostBuffers[i].destroy();
		}
	}
	
	CPUSharedPage::CPUSharedPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_) : BasePage(upperBoundEntrySize_, memoryBlockSize_, CPU_SHARED){}
};


