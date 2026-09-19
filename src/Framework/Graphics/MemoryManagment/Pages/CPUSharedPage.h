#pragma once 
#include "BasePage.h"

namespace Graphics
{
struct CPUSharedPage : BasePage 
{
	CPUSharedPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_);
};
}