#include "Image.h"
#include "Api.h"
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
	this->initialized = true; 
}

void Image::setImage(VkImage& srcImage_)
{
	this->vkHandle = srcImage_; 
}

void Image::addView(const char* name_, VkFormat format_, VkImageAspectFlags aspectFlags_, VkImageViewType dimensionType_, VkImageViewCreateFlags flags_)
{
	this->imageViews.emplace_back(name_, this->vkHandle, format_, aspectFlags_, dimensionType_, flags_); 
}

VkImage Image::getImage() const
{
    return this->vkHandle; 
}

VkImageView Image::getImageView(size_t id_) const
{
	return this->imageViews[id_].vkHandle;
}

VkImageCreateInfo Image::getImageMetadata() const
{
	return this->metadata; 
}


VkImageViewCreateInfo Image::getViewMetadata(size_t id_) const
{ 
	return this->imageViews[id_].metadata; 
}

void Image::destroy()
{
	this->imageViews.clear(); 
	vkDestroyImage(getMainDevice().logicalDevice, this->vkHandle, nullptr);
	this->vkHandle = VK_NULL_HANDLE; 
}
}
