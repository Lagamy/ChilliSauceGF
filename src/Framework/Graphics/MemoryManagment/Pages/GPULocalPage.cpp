#include "GPULocalPage.h"
#include "BasePage.h"
#include "Utilities.h"

namespace Graphics 
{
	void GPULocalPage::init()
	{
		BasePage::init();
		this->stagingHeaps.create(this->memoryBlockSize); 
	}
	
	void GPULocalPage::destroy()
	{
		BasePage::destroy(); 
		this->stagingHeaps.destroy(); 
	} 


	BufferCreationResult addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_, const char* uploadName_)
	{
		this->
	}

	GPULocalPage::GPULocalPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_) : BasePage(upperBoundEntrySize_, memoryBlockSize_, GPU_ONLY){}
};


