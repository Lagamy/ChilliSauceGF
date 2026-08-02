#include "Surface.h"
#include "Api.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void Surface::create() {
	// Creating surface createInfo struct, using built in GLFW function and runs vulkan create surface function)
	VkResult result = glfwCreateWindowSurface(getInstance().get(), getWindowPointer(), nullptr, &this->vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to crearte a surface!");
	}
}

void Surface::destroy() {
	vkDestroySurfaceKHR(getInstance().get(), this->vkHandle, nullptr);
	this->vkHandle = VK_NULL_HANDLE;
}

VkSurfaceKHR Surface::get() const
{
	return this->vkHandle;
}
}
