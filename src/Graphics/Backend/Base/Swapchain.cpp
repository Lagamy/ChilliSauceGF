#include "Swapchain.h"
#include "Framebuffer.h"
#include "Globals.h"
#include <array>
#include <cassert>
#include <cstddef>

namespace Graphics
{
// Presents render result as an Image to the Surface, which renders it on window.  
void Swapchain::create() {
	// Get Swapchain details - so we can pick best settings 
	GraphicsUtilities::SwapchainDetails swapchainDetails = GraphicsUtilities::getSwapchainDetails(Demo::renderer.mainDevice.physicalDevice);

	// Find optimal Surface values for our swapchain
	// 1. Choose best Swapchain Image Format 
	// 2. Choose best Presentation Mode
	// 3. Choose Swapchain Image Resolution 

	VkSurfaceFormatKHR swapchainImageFormat = GraphicsUtilities::chooseBestSurfaceFormat(swapchainDetails.imageFormats);
	VkPresentModeKHR presentationMode = GraphicsUtilities::chooseBestPresentationMode(swapchainDetails.presentationModes); // When to swap frames 
	VkExtent2D swapchainResolution = GraphicsUtilities::chooseSwapExtent(swapchainDetails.surfaceCapabilities);

	// How many images are in swapchain?
	uint32_t imageCount = swapchainDetails.surfaceCapabilities.minImageCount;

	if (swapchainDetails.surfaceCapabilities.maxImageCount == 0) // If 0 - means no limit for how many images swapchain can have
	{

	}
	// If there is space - get 1 more image to allow tripple buffering.  
	else if (imageCount <= swapchainDetails.surfaceCapabilities.maxImageCount)
	{
		imageCount += 1;
	}

	// Create swapchain for our Images 
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.imageFormat = swapchainImageFormat.format;
	swapchainCreateInfo.imageColorSpace = swapchainImageFormat.colorSpace;
	swapchainCreateInfo.presentMode = presentationMode;
	swapchainCreateInfo.imageExtent = swapchainResolution;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;													// Number of Layers for each image in chain
	swapchainCreateInfo.surface = Demo::renderer.surface.get();										// Surface
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;						// Means Image represents color of pixels(And not depth or stencil, or some custom user inputed mask)
	swapchainCreateInfo.preTransform = swapchainDetails.surfaceCapabilities.currentTransform;	// Additional transform to perform before present, like Rotation or Mirroring. Defined by our Surface. 
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;						// No alpha channel blending images with external graphics(e.g. other windows or wallpaper)
	swapchainCreateInfo.clipped = VK_TRUE;														// Don't draw part of image, that is not in view(e.g behind another window) 

	// Graphics queue will draw to those images, and Presentation Queue will Present them using Swapchain. 
	GraphicsUtilities::QueueFamilyIndicies indicies = GraphicsUtilities::getQueueFamilies(Demo::renderer.mainDevice.physicalDevice);
	// If Graphics and Presentation queue is same - we dont need to share image with other queues. 
	// Otherwise - it needs to be shared between 2 queues (Slower) 
	if (indicies.graphicsFamily == indicies.presentationFamily)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// 0 and nulltr, since we are not sharing this image with any other queue
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		uint32_t queueFamilyIndicies[] = {
			static_cast<uint32_t>(indicies.graphicsFamily),
			static_cast<uint32_t>(indicies.presentationFamily)
		};
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndicies;
	}
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // Pass responsability and all data from old Swapchain before destruction to this one.(Usefull when resizing the window) 

	// Create Swapchain
	VkResult result = vkCreateSwapchainKHR(Demo::renderer.mainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &this->vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Swapchain! VkResult = " + std::to_string(result));
	}

	this->imageFormat = swapchainImageFormat.format;
	this->extent = swapchainResolution;

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, &swapchainImageCount, nullptr);
	std::vector<VkImage> images(swapchainImageCount);
	vkGetSwapchainImagesKHR(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, &swapchainImageCount, images.data());

	this->renderTargets.resize(swapchainImageCount);
	for (uint32_t i = 0; i < images.size(); i++)
	{
		// Add initialized images to swapchain rendertargets 
		this->renderTargets[i].image.setImage(images[i]);
		this->renderTargets[i].image.addView("Swapchain", this->imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 0);
	}
	//printf("Swapchain Image Count: %u\n", swapchainImageCount);
}

void Swapchain::createFramebuffers(RenderPass& rRenderpass_) 
{ 
	for(Framebuffer& framebuffer : this->framebuffers) 
	{
		framebuffer.create(this->extent.width, this->extent.height, 1, rRenderpass_);
	}
}

void Swapchain::destroyFramebuffers()
{
	for(auto& framebuffer : framebuffers)
	{
		framebuffer.destroy();
	}
}

void Swapchain::destroy()
{
	for (auto& swapchainRenderTarget : this->renderTargets)
	{
		vkDestroyImageView(Demo::renderer.mainDevice.logicalDevice, swapchainRenderTarget.image.getImageView(0), nullptr);
		//vkDestroyImage(Globals::device.logicalDevice, swapchainImage.image, nullptr); // vkDestroySwapchainKHR also destroys images
	}
	vkDestroySwapchainKHR(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
	this->vkHandle = VK_NULL_HANDLE;
}

VkSwapchainKHR Swapchain::get() const {
	return this->vkHandle;
}
}
