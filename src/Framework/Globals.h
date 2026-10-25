#pragma once 

#include "Renderer.h"
#include "MemoryManager.h"
#include "ReflectionSystem.h"
// #include "AssetsManager.h"
#include "Utilities.h"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Globals {
    // - User Global components: 
    inline std::string processName = "PBRVulkanDemo"; 
    inline unsigned int windowWidth = 800;
    inline unsigned int windowHeight = 600;
    inline bool resizing = false; 
    inline uint64_t individualUpdateMemBlockSize; // Set in your setupEnvironmet func 
	inline std::vector<const char*> requiredDeviceExtensions = { // If you choose Ray tracing to be enabled -> this would be the thing to change
            VK_KHR_SWAPCHAIN_EXTENSION_NAME	
	};

    // - System Global components: 
    inline GLFWwindow* appWindow;
	inline Graphics::Renderer renderer;
	// inline AssetsManager assetsManager;
};

