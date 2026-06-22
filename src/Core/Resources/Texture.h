#pragma once 
#include "Image.h"

struct Texture { 
	Graphics::Image image; 
	uint32_t memoryEntryId; 
	std::vector<uint8_t> bitmap; 
};
