#pragma once 
#include <vulkan/vulkan.h>
#include <stdio.h>

namespace Graphics
{
struct DebugMessenger { 
	VkDebugUtilsMessengerCreateInfoEXT createInfo;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    	VkDebugUtilsMessageTypeFlagsEXT type,
    	const VkDebugUtilsMessengerCallbackDataEXT* data,
    	void* userData)
	{
    	printf("Validation: %s\n", data->pMessage);
		return VK_FALSE;
	}	
	
	void init(); 
};
}
