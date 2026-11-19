#pragma once
#include "PoolId.h"
#include "Utilities.h"
#include <cstdint>

namespace Graphics
{
	/* Called by the engine to fill a descriptor that was loaded again, with the same functions that wrote it the first time (DescriptorSet::setDescriptorMemberValue...). setId_ and descriptorId_ are the ids Api.h functions take. */
	using DescriptorLoadFunc = void (*)(PoolId setId_, PoolId descriptorId_);

	/* What a draw or a ONESHOT pass does while a descriptor it uses is not in memory yet. Both modes start the load at the next frame start, a request is never served inside the frame that made it. */
	enum DescriptorLoadModeEnum
	{
		SKIP_UNTIL_LOADED, // the draw is skipped (a ONESHOT pass is held back) until the upload completed, the CPU never waits
		WAIT_UNTIL_LOADED // the CPU waits right after the frame's submit until the upload of the load completed, so the next frame can use it at once. Blocks the calling thread, meant for a renderer that runs on its own thread
	};

	/*
		Everything needed to create a descriptor set, filled field by field like a Vulkan create info (start from `= {}`). Used by addDescriptorSet. validate() throws on a missing or contradicting field.

		layoutId, allocatorType, memoryVisability: required.
		absenceSecondsCount: the engine unloads a descriptor of the set (its memory entries are removed) once no draw or pass used it for this many seconds, and loads it again when one does. at least 1 (0 throws), UninitializedId = never unloaded automatically (still by unloadDescriptor, cleanup and scene switch). Only DYNAMIC_ALLOCATOR sets can unload, since static entries can't be removed: a static set has to set UninitializedId.
		loadFromDisk: optional. Set it when the application has an asset pipeline: the engine calls it after a load to write the descriptor again and keeps no CPU copy. Without it the set keeps a CPU copy of every byte written to its descriptors (as large as the descriptors) and writes it back on a load.
		loadMode: see DescriptorLoadModeEnum.
		Every setting applies to all descriptors of the set, the load and unload itself is per descriptor.
	*/
	struct DescriptorSetCreateParams
	{
		PoolId layoutId = UninitializedPoolId;
		AllocatorTypeEnum allocatorType = ALLOCATOR_TYPE_UNSET;
		MemoryVisabilityEnum memoryVisability = MEMORY_VISABILITY_UNSET;
		uint32_t absenceSecondsCount = 1;
		DescriptorLoadFunc loadFromDisk = nullptr;
		DescriptorLoadModeEnum loadMode = SKIP_UNTIL_LOADED;

		void validate() const; // throws std::runtime_error
	};
}
