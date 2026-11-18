#pragma once
#include "Utilities.h"
#include <cstdint>

namespace Graphics
{
	/*
		How an entry is accessed after creation, and how many copies of its memory that needs. Filled field by field like a Vulkan create info (start from `= {}`), validate() throws on a combination that makes no sense.

		accessMode (required):
		- READ_ONLY: one instance in memory that every frame in flight reads at any time. copyCountMode has to stay CopyCountModeEnum::ONE.
		- READ_AND_WRITE: the entry can change after creation.

		copyCountMode (conditionally required): see CopyCountModeEnum. A READ_AND_WRITE CPU_SHARED entry has to choose FOLLOW_FRAMES_IN_FLIGHT or CUSTOM_COPY_COUNT.
		A READ_AND_WRITE GPU_LOCAL entry may choose any mode (ONE keeps one device buffer written through staging), a READ_ONLY entry has to leave it at CopyCountModeEnum::ONE.

		customCopyCount (conditionally required): the copy count of CUSTOM_COPY_COUNT, at least 1. It has to stay 0 for every other mode.
	*/
	struct MemoryAccessParams
	{
		MemoryAccessModeEnum accessMode = MemoryAccessModeEnum::UNSET;
		CopyCountModeEnum copyCountMode = CopyCountModeEnum::ONE;
		uint32_t customCopyCount = 0;

		bool isReadOnly() const;
		uint32_t resolveCopyCount() const; // 1 for CopyCountModeEnum::ONE, framesAtFlightCount for FOLLOW_FRAMES_IN_FLIGHT, customCopyCount for CUSTOM_COPY_COUNT
		void validate(const char* entryName_, MemoryVisabilityEnum memoryVisability_) const; // throws std::runtime_error
	};
}
