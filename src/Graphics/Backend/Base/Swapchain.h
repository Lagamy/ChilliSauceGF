#pragma once 
#include "Utilities.h"
#include "Image.h"

struct Swapchain {
	VkSwapchainKHR vkHandle = VK_NULL_HANDLE; // Handle 
	VkFormat imageFormat;
	VkExtent2D extent; // Resolution  

	std::vector<Image> swapchainImages;

	void create();
	void destroy();
	VkSwapchainKHR get() const;
};
