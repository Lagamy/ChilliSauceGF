/*
Image - is physical layout of image that would define how much memory it will use and what exactly will be stored there. 

Image has 1 to many relation ship with ImageView. You can have one image and multiple views for different shaders or purposes.

Examples:

Cubemap:
	VkImage -> 6 layers (cube faces)
	VkImageView -> 6 layers, viewType = CUBE
	VkImageView -> just 1 face, viewType = 2D

Depth/stencil separation:
	VkImage -> D32_SFLOAT_S8_UINT
	VkImageView 1 -> depth-only, D32_SFLOAT
	VkImageView 2 -> stencil-only, S8_UINT
*/

#pragma once

#include "Semaphore.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
// enum AdditionalUsageEnum : unsigned int {
	// SAMPLED = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT,
	// STORAGE = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT
// };
namespace Graphics
{
struct Image {
	VkImage vkHandle = VK_NULL_HANDLE; // How data will be laid out in memory. 
	VkImageCreateInfo metadata; 
	bool allocated = false; 
	bool initialized = false;
	
	// ImageViews - How to read/interprate data.
	std::vector<std::string> viewNames; 
	std::vector<VkImageView> viewHandles; 
	std::vector<VkImageViewCreateInfo> viewMetadatas; 
	Semaphore imageInUseSemaphoreFinished; 

	void init(VkImageUsageFlags usageFlags_, VkFormat format_, VkExtent3D extent_, VkImageType imageType_, uint32_t mipLevelCount_, uint32_t arrayLayerCount_, VkImageCreateFlags flags_, bool cpuBitmapEdits_);
	void setImage(VkImage& srcImage_); // Needed for swapchain(We dont create images there - we borrow them from swapchain itself)  
	void addView(const char* name_, VkFormat format_, VkImageAspectFlags aspectFlags_, VkImageViewType dimensionType_, VkImageViewCreateFlags flags_);
	void destroyViews();
	VkSemaphore& getInUseSemaphoreFinished();
	void destroy();

	VkImageView getImageView(size_t id_) const;
	VkImage get() const;
};
}
