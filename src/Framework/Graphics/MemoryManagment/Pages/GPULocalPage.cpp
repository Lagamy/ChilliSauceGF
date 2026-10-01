#include "GPULocalPage.h"
#include "BasePage.h"
#include "Utilities.h"

namespace Graphics 
{
	void GPULocalPage::init()
	{
		std::array<Buffer, BufferTypesCount> ghostBuffers; 
		ghostBuffers[0].create(1,VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Index memType inclusion in memReqs"); 
		ghostBuffers[1].create(1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Vertex memType inclusion in memReqs"); 
		ghostBuffers[2].create(1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Uniform memType inclusion in memReqs"); 
		ghostBuffers[3].create(1,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Ghost buffer for Storage memType inclusion in memReqs"); 
	
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
	
	GPULocalPage::GPULocalPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_, uint32_t maxDormantStagingHeaps_) : BasePage(upperBoundEntrySize_, memoryBlockSize_, GPU_ONLY, maxDormantStagingHeaps_){}
};


