#include "ImagePack.h"
#include "Api.h"
#include "Globals.h"
#include <format>
#include <stdexcept>

namespace Graphics
{
ImagePack::ImagePack(bool perFrameInFlight_)
{
	this->perFrameInFlight = perFrameInFlight_;
}

void ImagePack::init(const char* name_, VkImageUsageFlags usageFlags_, VkFormat format_, VkExtent3D extent_, VkImageType imageType_, uint32_t mipLevelCount_, uint32_t arrayLayerCount_, VkImageCreateFlags flags_, bool cpuBitmapEdits_)
{
	if(!this->images.empty())
	{
		throw std::runtime_error("Image Pack: init called twice.");
	}

	uint32_t copyCount = this->perFrameInFlight ? Globals::renderer.framesAtFlightCount : 1;
	this->images.resize(copyCount);
	for(Image& rImage : this->images)
	{
		rImage.initMetadata(name_, usageFlags_, format_, extent_, imageType_, mipLevelCount_, arrayLayerCount_, flags_, cpuBitmapEdits_);
		rImage.create(); 
	}
}

void ImagePack::destroy()
{
	for(Image& rImage : this->images)
	{
		rImage.destroy();
	}
	this->images.clear();
}

/* The copy of the current frame if the pack is per frame in flight, otherwise the only one. */
Image& ImagePack::get()
{
	return this->get(this->perFrameInFlight ? getCurrentFrameIndex() : 0);
}

Image& ImagePack::get(uint32_t frame_)
{
	#ifdef ENGINE_DEBUG
		if(frame_ >= this->images.size())
		{
			throw std::runtime_error(std::format("Image Pack: copy {} requested, it has {} (init it first).", frame_, this->images.size()));
		}
	#endif
	return this->images[frame_];
}
}
