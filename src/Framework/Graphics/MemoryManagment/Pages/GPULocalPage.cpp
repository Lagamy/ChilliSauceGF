#include "GPULocalPage.h"
#include "BasePage.h"
#include "Utilities.h"

namespace Graphics 
{
	void GPULocalPage::init()
	{
		BasePage::init();
		this->stagingHeap.create(this->memoryBlockSize); 
	}
	
	void GPULocalPage::destroy()
	{
		BasePage::destroy(); 
		this->stagingHeap.destroy(); 
	} 

	GPULocalPage::GPULocalPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_) : BasePage(upperBoundEntrySize_, memoryBlockSize_, GPU_ONLY){}
};


