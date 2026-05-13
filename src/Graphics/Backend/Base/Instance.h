#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <glfw/glfw3.h>
#include <string.h>
#include <stdexcept>

struct Instance {
private:
	VkInstance vkHandle = VK_NULL_HANDLE;
public:
	void setup();
	void destroy();
	bool checkInstanceExtensionSupport(std::vector<const char*>* extensionsForCheck_);

	VkInstance get() const;
};
