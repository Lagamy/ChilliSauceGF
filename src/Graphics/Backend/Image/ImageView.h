// How to read/interprate VkImage. 
/* 
	Describes what part of Image and which format - you see it as.
*/
#pragma once 
#include <vulkan/vulkan.h>
#include <string>

namespace Graphics
{
struct ImageView {
	std::string name; 
	VkImageView vkHandle; 
	VkImageViewCreateInfo metadata; 

	ImageView(const char* name_, VkImage& rImage_, VkFormat format_, VkImageAspectFlags aspectFlags_, VkImageViewType dimensionType_, VkImageViewCreateFlags flags_);
	~ImageView();
};
}
