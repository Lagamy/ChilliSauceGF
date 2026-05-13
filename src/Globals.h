#pragma once 

#include "Renderer.h"
#include "GPUMemoryManager.h"
#include "AssetsManager.h"
#include <glfw/glfw3.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace App {
    // - User Global components: 
    inline std::string processName = "PBRVulkanDemo";
    // - Window/screen resolution 
    inline unsigned int windowWidth = 800;
    inline unsigned int windowHeight = 600;

    // - System Global components: 
    inline GLFWwindow* appWindow;
};

namespace Demo { 
	inline std::vector<const char*> requiredDeviceExtensions = { // If you choose Ray tracing to be enabled -> this would change
            VK_KHR_SWAPCHAIN_EXTENSION_NAME	
	}; 
	inline GPUMemoryManager GPUMemoryManager;
	inline Renderer renderer;
	inline AssetsManager assetsManager;
};

