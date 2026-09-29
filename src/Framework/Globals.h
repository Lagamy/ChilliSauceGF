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
    inline uint32_t maxDormantUpdateStagingHeaps; 
    inline uint32_t minDormantUpdateStagingHeaps; 
    inline uint32_t maxAttemptsToUseExistingStagingHeaps; // before trying empty or creating new once. Here i can do that instead of paging. Cause after upload/update all heaps are empty anyway. “How much CPU you willing to spend avoiding another staging heap?”

    // - System Global components: 
    inline GLFWwindow* appWindow;
	inline Graphics::Renderer renderer;
	// inline AssetsManager assetsManager;
};

