#pragma once 

#include "Renderer.h"
#include "GPUMemoryManager.h"
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


    // - System Global components: 
    inline GLFWwindow* appWindow;
	inline Graphics::Renderer renderer;
	// inline AssetsManager assetsManager;
};

