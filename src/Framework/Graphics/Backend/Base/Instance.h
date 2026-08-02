#pragma once
#include "ValidationLayers/ValidationLayers.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdexcept>

namespace Graphics
{
struct Instance {
private:
	VkInstance vkHandle = VK_NULL_HANDLE;
	ValidationLayers validationLayers; 
public:
	void setup();
	void destroy();
	bool checkInstanceExtensionSupport(std::vector<const char*>* extensionsForCheck_);

	VkInstance get() const;
};
}
