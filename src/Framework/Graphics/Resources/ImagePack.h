#pragma once

#include "Image.h"
#include <vector>

namespace Graphics
{
/*
ImagePack - the images one application level image is made of: one when the image is the same for every frame in flight, framesAtFlightCount of them (same parameters, own memory) when it is per frame in flight.
Same idea as the per frame copies of a descriptor: get() returns the copy of the current frame, so frames in flight never touch the same VkImage.
*/
struct ImagePack
{
	std::vector<Image> images;
	bool perFrameInFlight;

	ImagePack(bool perFrameInFlight_);

	void init(const char* name_, VkImageUsageFlags usageFlags_, VkFormat format_, VkExtent3D extent_, VkImageType imageType_, uint32_t mipLevelCount_, uint32_t arrayLayerCount_, VkImageCreateFlags flags_, bool cpuBitmapEdits_);
	void destroy();

	Image& get();
	Image& get(uint32_t frame_);
};
}
