#pragma once 
#include "DebugMessenger.h"
#include <vulkan/vulkan.h>
#include <array>

struct ValidationLayers { 
	DebugMessenger debugMessenger; 
	std::array<const char*, 1> layers = {
    	"VK_LAYER_KHRONOS_validation"
	};

	std::array<const char*, 2> extensions = {
    	VK_KHR_SURFACE_EXTENSION_NAME,
    	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
};
