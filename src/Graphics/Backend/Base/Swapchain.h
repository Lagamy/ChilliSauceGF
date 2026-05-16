#pragma once 

#include "Framebuffer.h"
#include "RenderPass.h"
#include "Utilities.h"
#include "Image.h"

struct Swapchain {
	VkSwapchainKHR vkHandle = VK_NULL_HANDLE; // Handle 
	VkFormat imageFormat;
	VkExtent2D extent; // Resolution  

	std::vector<Image> images;
	std::vector<Framebuffer> framebuffers;

	void create();
	void createFramebuffers(RenderPass& rRenderpass_); 
	void destroy();
	VkSwapchainKHR get() const;
};
