#include "CPUSharedPage.h"

namespace Graphics 
{
	CPUSharedPage::CPUSharedPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_) : BasePage(upperBoundEntrySize_, memoryBlockSize_, CPU_SHARED){}
};


