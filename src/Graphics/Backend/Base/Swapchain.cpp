#include "Swapchain.h"
#include "Framebuffer.h"
#include "Api.h"
#include <array>
#include <cassert>
#include <cstddef>

namespace Graphics
{
// Presents render result as an Image to the Surface, which renders it on window.  
void Swapchain::create() {
	// Get Swapchain details - so we can pick best settings 
	SwapchainDetails swapchainDetails = getSwapchainDetails(getMainDevice().physicalDevice);

	// Find optimal Surface values for our swapchain
	// 1. Choose best Swapchain Image Format 
	// 2. Choose best Presentation Mode
	// 3. Choose Swapchain Image Resolution 

	VkSurfaceFormatKHR swapchainImageFormat = chooseBestSurfaceFormat(swapchainDetails.imageFormats);
	VkPresentModeKHR presentationMode = chooseBestPresentationMode(swapchainDetails.presentationModes); // When to swap frames 
	VkExtent2D swapchainResolution = chooseSwapExtent(swapchainDetails.surfaceCapabilities);

	// How many images are in swapchain?
	uint32_t imageCount = swapchainDetails.surfaceCapabilities.minImageCount;

	// If there is space - get 1 more image to allow tripple buffering.  
	if (imageCount <= swapchainDetails.surfaceCapabilities.maxImageCount)
	{
		imageCount += 1;
	}

	setFramesAtFlightCount(imageCount);

	// Create swapchain for our Images 
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.imageFormat = swapchainImageFormat.format;
	swapchainCreateInfo.imageColorSpace = swapchainImageFormat.colorSpace;
	swapchainCreateInfo.presentMode = presentationMode;
	swapchainCreateInfo.imageExtent = swapchainResolution;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;													// Number of Layers for each image in chain
	swapchainCreateInfo.surface = getSurface().get();										// Surface
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;						// Means Image represents color of pixels(And not depth or stencil, or some custom user inputed mask)
	swapchainCreateInfo.preTransform = swapchainDetails.surfaceCapabilities.currentTransform;	// Additional transform to perform before present, like Rotation or Mirroring. Defined by our Surface. 
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;						// No alpha channel blending images with external graphics(e.g. other windows or wallpaper)
	swapchainCreateInfo.clipped = VK_TRUE;														// Don't draw part of image, that is not in view(e.g behind another window) 

	// Graphics queue will draw to those images, and Presentation Queue will Present them using Swapchain. 
	QueueFamilyIndices indices = getQueueFamilies(getMainDevice().physicalDevice);
	// If Graphics and Presentation queue is same - we dont need to share image with other queues. 
	// Otherwise - it needs to be shared between 2 queues (Slower) 
	if (indices.graphicsFamily == indices.presentationFamily)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// 0 and nulltr, since we are not sharing this image with any other queue
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		uint32_t queueFamilyIndices[] = {
			static_cast<uint32_t>(indices.graphicsFamily),
			static_cast<uint32_t>(indices.presentationFamily),
			static_cast<uint32_t>(indices.transferFamily),
			static_cast<uint32_t>(indices.computeFamily)
		};
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // Pass responsability and all data from old Swapchain before destruction to this one.(Usefull when resizing the window) 

	// Create Swapchain
	VkResult result = vkCreateSwapchainKHR(getMainDevice().logicalDevice, &swapchainCreateInfo, nullptr, &this->vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Swapchain! VkResult = " + std::to_string(result));
	}

	this->imageFormat = swapchainImageFormat.format;
	this->extent = swapchainResolution;

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(getMainDevice().logicalDevice, this->vkHandle, &swapchainImageCount, nullptr);
	std::vector<VkImage> images(swapchainImageCount);
	vkGetSwapchainImagesKHR(getMainDevice().logicalDevice, this->vkHandle, &swapchainImageCount, images.data());

	this->renderTargets.resize(swapchainImageCount);
	this->imageUseFinishedSemaphoreIds.resize(swapchainImageCount); 
	std::ostringstream name;
	for (uint32_t i = 0; i < images.size(); i++)
	{
		name.clear(); 
		// Add initialized images to swapchain rendertargets 
		this->renderTargets[i].setImage(images[i]);
		this->renderTargets[i].addView("Swapchain", this->imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 0);
		name << "Swapchain Image " << i << " Use Finished";
		this->imageUseFinishedSemaphoreIds[i] = addSemaphore(name.str().c_str()); 
	}
	// Create uninitialized framebuffers corresponding to those images 
	this->framebuffers.resize(swapchainImageCount);
}

void Swapchain::createFramebuffers(RenderPass& rRenderpass_) 
{ 
	for(uint32_t i = 0; i < this->framebuffers.size(); i++) 
	{
		this->framebuffers[i].create(this->extent.width, this->extent.height, 1, this->renderTargets[i].viewHandles, rRenderpass_);
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
		vkDestroyImageView(getMainDevice().logicalDevice, swapchainRenderTarget.getImageView(0), nullptr);
		//vkDestroyImage(Globals::device.logicalDevice, swapchainImage.image, nullptr); // vkDestroySwapchainKHR also destroys images
	}
	vkDestroySwapchainKHR(getMainDevice().logicalDevice, this->vkHandle, nullptr);
	this->vkHandle = VK_NULL_HANDLE;
}

VkSwapchainKHR Swapchain::get() const {
	return this->vkHandle;
}
}
