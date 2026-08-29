#pragma once 
#include "UploadEntry.h"
#include "Utilities.h"

namespace  Graphics 
{
struct UploadEntryGroup 
{
	std::array<std::vector<UploadEntry>, BufferTypesCount> entriesPerBufferType;
    std::vector<UploadEntry>& operator[](BufferTypeEnum bufferType_) { return this->entriesPerBufferType[bufferType_]; }
};
}
