#pragma once 
#include "Utilities.h"

namespace Graphics 
{
struct UpdateEntry 
{ 
	uint32_t entryId; 
	BufferTypeEnum uploadType; 
	size_t byteAmount = 0; 
	size_t heapStartingByte = 0; 
	size_t dstStartingByte = 0; 
	AllocatorTypeEnum allocatorType; 

	bool partialUpdate;

	UpdateEntry(uint32_t entryId_, BufferTypeEnum uploadType_, const void* data_, size_t heapStartingByte_); 
	UpdateEntry(uint32_t entryId_, BufferTypeEnum uploadType_, const void* data_, size_t heapStartingByte_, size_t byteAmount_, size_t srcStartingbyte_);
};
}

