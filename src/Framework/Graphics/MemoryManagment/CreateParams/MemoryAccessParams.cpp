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

	uint32_t MemoryAccessParams::resolveCopyCount() const
	{
		if(this->copyCountMode == CopyCountModeEnum::FOLLOW_FRAMES_IN_FLIGHT)
		{
			return Globals::renderer.framesAtFlightCount;
		}
		if(this->copyCountMode == CopyCountModeEnum::CUSTOM_COPY_COUNT)
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

		/* A CPU_SHARED write goes straight into memory the GPU may be reading, so it has to choose how many copies hide that. A GPU_LOCAL write goes through staging and doesn't block, so ONE is legal there (copies trade memory for newer data per frame). */
		bool needsCopyCountMode = this->accessMode == MemoryAccessModeEnum::READ_AND_WRITE && memoryVisability_ == CPU_SHARED;
		if(needsCopyCountMode && this->copyCountMode == CopyCountModeEnum::ONE)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} is READ_AND_WRITE and CPU_SHARED, it needs a copyCountMode (FOLLOW_FRAMES_IN_FLIGHT or CUSTOM_COPY_COUNT).", entryName_));
		}
		if(this->accessMode == MemoryAccessModeEnum::READ_ONLY && this->copyCountMode != CopyCountModeEnum::ONE)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} is READ_ONLY, it is one instance every frame reads. Leave copyCountMode at CopyCountModeEnum::ONE.", entryName_));
		}

		if(this->copyCountMode == CopyCountModeEnum::CUSTOM_COPY_COUNT && this->customCopyCount == 0)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} uses CUSTOM_COPY_COUNT, customCopyCount has to be at least 1.", entryName_));
		}
		if(this->copyCountMode != CopyCountModeEnum::CUSTOM_COPY_COUNT && this->customCopyCount != 0)
		{
			throw std::runtime_error(std::format("Memory Access Params: Entry {} sets customCopyCount, but its copyCountMode is not CUSTOM_COPY_COUNT.", entryName_));
		}
	}
}
