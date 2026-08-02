// A connection between Vulkan and this systems Window.(Window/MacOs/Linux/SwitchOs..). Swapchain will present images to it -> GLFW will read those images from it. 
#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Graphics
{
struct Surface {
	VkSurfaceKHR vkHandle = VK_NULL_HANDLE;

	void create();
	void destroy();

	VkSurfaceKHR get() const;
};
}
