#include "MemoryAccessParams.h"
#include "Globals.h"
#include <format>
#include <stdexcept>

namespace Graphics
{
	bool MemoryAccessParams::isReadOnly() const
	{
		return this->accessMode == MemoryAccessModeEnum::READ_ONLY;
	}

	bool MemoryAccessParams::hasShadow() const
	{
		return this->resolveCopyCount() > 1;
	}

	uint32_t MemoryAccessParams::resolveCopyCount() const
	{
		if(this->copyCountMode == CopyCountModeEnum::FOLLOW_FRAMES_IN_FLIGHT)
		{
			return Globals::renderer.framesAtFlightCount;
		}
		if(this->copyCountMode == CopyCountModeEnum::CUSTOM_COPY_COUNT_WITH_WAIT)
		{
			return this->customCopyCount;
		}
		return 1;
	}

	/* The rules are checked in release too: they are cheap, run once per entry, and a wrong combination would silently give the entry a different memory model than the caller meant. */
	void MemoryAccessParams::validate(const char* entryName_, MemoryVisabilityEnum memoryVisability_) const
	{
		if(this->accessMode == MemoryAccessModeEnum::UNSET)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} has no accessMode, set READ_ONLY or READ_AND_WRITE.", entryName_));
		}

		if(this->accessMode == MemoryAccessModeEnum::READ_ONLY && this->copyCountMode != CopyCountModeEnum::ONE)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} is READ_ONLY, it is one instance every frame reads. Leave copyCountMode at CopyCountModeEnum::ONE.", entryName_));
		}

		if(this->copyCountMode == CopyCountModeEnum::CUSTOM_COPY_COUNT_WITH_WAIT && this->customCopyCount == 0)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} uses CUSTOM_COPY_COUNT_WITH_WAIT, customCopyCount has to be at least 1.", entryName_));
		}
		if(this->copyCountMode == CopyCountModeEnum::CUSTOM_COPY_COUNT_WITH_WAIT && this->customCopyCount > Globals::renderer.framesAtFlightCount)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} has customCopyCount {}, more than the {} frames in flight. The extra copies would never be read, use FOLLOW_FRAMES_IN_FLIGHT or a smaller count.", entryName_, this->customCopyCount, Globals::renderer.framesAtFlightCount));
		}
		if(this->copyCountMode != CopyCountModeEnum::CUSTOM_COPY_COUNT_WITH_WAIT && this->customCopyCount != 0)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} sets customCopyCount, but its copyCountMode is not CUSTOM_COPY_COUNT_WITH_WAIT.", entryName_));
		}
	}
}
