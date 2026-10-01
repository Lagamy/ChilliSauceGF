#pragma once 
#include "BasePage.h"

namespace Graphics
{
struct CPUSharedPage : BasePage 
{
	void init(); 
	CPUSharedPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_, uint32_t maxDormantStagingHeaps_);
};
}