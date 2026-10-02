#pragma once 
#include "Image.h"

namespace Graphics 
{
struct Texture 
{ 
	Graphics::Image image; 
	uint32_t memoryEntryId; 
	std::vector<uint8_t> bitmap; 
	uint32_t width, height, channels;
};
}