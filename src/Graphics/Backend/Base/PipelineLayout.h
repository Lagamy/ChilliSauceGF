// Exists to incapsulate every Descriptor sets / Push constants layout. So Graphics Pipeline knows - what it will work with.   
#pragma once 
#include <vulkan/vulkan.h>
#include <format>

struct PipelineLayout {
private:
	VkPipelineLayout vkHandle = VK_NULL_HANDLE;
public:
	void create(); // Temporary hardcoded
	void destroy();
	VkPipelineLayout get() const;
};
