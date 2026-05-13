#include "ImageView.h"
#include "Globals.h"

ImageView::ImageView(const char* name_, VkImage& rImage_, VkFormat format_, VkImageAspectFlags aspectFlags_, VkImageViewType dimensionType_, VkImageViewCreateFlags flags_) {
	metadata = {};
	metadata.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	metadata.image = rImage_;
	metadata.viewType = dimensionType_; // Dimensions of image(1d for line, 2d for textures, 3d for voluometric, etc). How to interpret image memory. 

	/*
	VkImageView.format allows reinterpretation of the image's data in some cases.

	Example cases :
		SRGB vs UNORM:
			Image stored as VK_FORMAT_R8G8B8A8_UNORM
			I want a shader to read it as linear(no gamma correction): VK_FORMAT_R8G8B8A8_SRGB

		Depth / stencil views:
			Image is VK_FORMAT_D32_SFLOAT_S8_UINT
			I want a depth - only view: VK_FORMAT_D32_SFLOAT
			Or stencil - only view: VK_FORMAT_S8_UINT

		Typeless or mutable formats:
			Image created with VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT
			You can make multiple views with compatible formats:
				For example, a 32 - bit float image can be viewed as R32_SFLOAT or R32_UINT
	*/
	metadata.format = format_;
	
	/*	
	Lets me enable special capabilities for the image view.
	Example usage: 

	VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DYNAMIC_BIT_EXT:
		For fragment density maps (used in VR / foveated rendering)
		Allows the view to be updated dynamically

	VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DEFERRED_BIT_EXT:
		Works with the dynamic flag to defer certain updates
	*/

	metadata.flags = flags_;
	// Swiziling 
	metadata.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Allows remaping of rgba vars to other rgba values(make it so r = blue channel) 
	metadata.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	metadata.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	metadata.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	// Subresources allow the view to view only a part of an - image 
	metadata.subresourceRange.aspectMask = aspectFlags_; // Which aspect of image to view (e.g. COLOR_BIT for viewing color, DEPTH_BIT for viewing depth, etc)
	metadata.subresourceRange.baseMipLevel = 0; // Start mipmap level to view 
	metadata.subresourceRange.levelCount = 1; // Only 1 level for now. (Till i implement linear filtering)
	metadata.subresourceRange.baseArrayLayer = 0; // Start array level to view
	metadata.subresourceRange.layerCount = 1; // Amount of layers image has. 
	// Create image view and return it 
	VkImageView imageView;
	VkResult result = vkCreateImageView(Demo::renderer.mainDevice.logicalDevice, &metadata, nullptr, &imageView);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}
	this->vkHandle = imageView;
	this->name = name_; 
}

ImageView::~ImageView()
{
	vkDestroyImageView(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
}
