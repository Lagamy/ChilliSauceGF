#include "Residency.h"
#include "Globals.h"
#include <stdexcept>

namespace Graphics
{
	void Residency::markUsedFrame()
	{
		this->lastUsedFrame = Globals::renderer.frameNumber;
	}

	void ResidencyParams::validate(AllocatorTypeEnum allocatorType_) const
	{
		if(this->absenceSecondsCount == 0)
		{
			throw std::runtime_error("Residency Params: absenceSecondsCount can't be 0, it would unload and load again every frame. Use 1 or more, or UninitializedId to never unload.");
		}
		if(this->loadFromDisk != nullptr && (this->saveToDisk == nullptr || this->changeLayoutOnDisk == nullptr))
		{
			throw std::runtime_error("Residency Params: loadFromDisk needs saveToDisk and changeLayoutOnDisk too.");
		}
		if(this->loadFromDisk == nullptr && (this->saveToDisk != nullptr || this->changeLayoutOnDisk != nullptr))
		{
			throw std::runtime_error("Residency Params: saveToDisk and changeLayoutOnDisk belong to loadFromDisk, set it too.");
		}
		if(allocatorType_ == STATIC_ALLOCATOR && this->absenceSecondsCount != UninitializedId)
		{
			throw std::runtime_error("Residency Params: A STATIC_ALLOCATOR owner can't unload (static entries can't be removed), set absenceSecondsCount to UninitializedId.");
		}
	}
}
