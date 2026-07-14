#include "Image.h"
#include "Api.h"
#include <vulkan/vulkan_core.h>
/*
	createImage() does not allocate memory, and youll need vkAllocateMemory + vkBindImageMemory before using the image.
*/

namespace Graphics
{
void Image::init(VkImageUsageFlags usageFlags_, VkFormat format_, VkExtent3D extent_, VkImageType imageType_, uint32_t mipLevelCount_, uint32_t arrayLayerCount_, VkImageCreateFlags flags_, bool cpuBitmapEdits_) {
	metadata = {}; 
	metadata.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	metadata.pNext = NULL; // is NULL or a pointer to a structure extending this structure.
	metadata.format = format_; // Texel format
	metadata.imageType = imageType_; // Is it 1D, 2D or 3D. How to store it in memory. Physical dimensionality
	metadata.extent = extent_; // Size of image in all available dimensions.
	// Example of each Mip level extent: 1024 -> 512 -> 256 -> 128 -> 64 -> 32 -> 16 -> 8 -> 4 -> 2 -> 1
	metadata.mipLevels = mipLevelCount_; // Count of how much downsampled versions of this image(mips) will be generated for use(/2, /4, /8, etc.). Using formula: max(1, floor(previousMip / 2))

	/* arrayLayers - how many separate images of the same size and format are stored in this one VkImage.
	
	Each layer:
		has the same format
		has the same extent
		has its own mip level selected
		can be rendered to or sampled independently

	Use cases: 
		arrayLayers = 1 -> normal image
		arrayLayers = 6 -> cubemap (6 faces)
		arrayLayers = 4 -> 2D texture array with 4 slices
		arrayLayers = N -> N shadow maps, N G-buffer targets, N animation frames, etc.
	*/

	metadata.arrayLayers = arrayLayerCount_; // How many layers are in this image 
	// Storage conceptually goes as follow: [layer][mip][(z if 3D)]

	/* 
	Layouts control this stuff:
		whether the image is in a form suitable for sampling
		whether it can be used as a render target
		whether it can be used as a transfer destination
		whether it is owned by the presentation engine

	Examples:
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	*/
	metadata.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Leave it to the render pass / subpass.
	metadata.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Image is used by 1 queue at a time. Need to explicitly transfer ownership if another queue family wants to use it.
	
	/*
	Unlocks special capabilities for this image.
	Example usage:

	VK_IMAGE_CREATE_SPARSE_BINDING_BIT:
		The images memory can be bound in pieces instead of all at once.
		Used for sparse / virtual textures and very large resources.

	VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT:
		Only parts of the image need to be resident in memory.
		Lets you load/unload tiles of a texture.

	VK_IMAGE_CREATE_SPARSE_ALIASED_BIT:
		Different sparse images can share the same memory.

	VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT:
		The image can be viewed with different formats.
		Example: create an image as R8G8B8A8, but make a view that treats it as R8G8B8A8_SRGB.

	VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:
		Allows the image to be used as a cube map.
		Required if you want VK_IMAGE_VIEW_TYPE_CUBE or CUBE_ARRAY.

	VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT:
		Lets a 3D image be viewed as a 2D array.

	VK_IMAGE_CREATE_ALIAS_BIT:
		The image can alias memory with another image.
		Often used with memory suballocation or advanced resource management.

	VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT:
		Used for multi-GPU setups.
		Allows different GPUs to back different parts of the image.
	
	*/
	metadata.flags = flags_;
	metadata.tiling = cpuBitmapEdits_ ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL; // VK_IMAGE_TILING_OPTIMAL = GPU driver chooses best memory layout for GPU side performance. 
	metadata.usage = usageFlags_ /* | additionalUsageFlags_ */;
	this->imageInUseSemaphoreFinished.create(); 
	this->initialized = true; 
}

VkSemaphore& Image::getInUseSemaphoreFinished()
{
	return this->imageInUseSemaphoreFinished.get();
}

void Image::setImage(VkImage& srcImage_)
{
	this->vkHandle = srcImage_; 
}

void Image::addView(const char* name_, VkFormat format_, VkImageAspectFlags aspectFlags_, VkImageViewType dimensionType_, VkImageViewCreateFlags flags_)
{
	
	VkImageViewCreateInfo metadata = {};
	metadata.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	metadata.image = this->vkHandle;
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
	VkResult result = vkCreateImageView(getMainDevice().logicalDevice, &metadata, nullptr, &imageView);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}
	this->viewHandles.emplace_back(imageView); 
	this->viewMetadatas.emplace_back(metadata); 
	this->viewNames.emplace_back(name_); 
}


VkImageView Image::getImageView(size_t id_) const
{
	return this->viewHandles[id_];
}

VkImage Image::get() const  
{ 
	return this->vkHandle; 
}

void Image::destroyViews()
{
	for (VkImageView view : viewHandles)
	{
    	vkDestroyImageView(getMainDevice().logicalDevice, view, nullptr);
	}

	this->viewHandles.clear();
	this->viewMetadatas.clear();
	this->viewNames.clear();
}

void Image::destroy()
{
	this->destroyViews();
	this->imageInUseSemaphoreFinished.destroy(); 
	vkDestroyImage(getMainDevice().logicalDevice, this->vkHandle, nullptr);
	this->vkHandle = VK_NULL_HANDLE; 
}
}
