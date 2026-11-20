#pragma once
#include "PoolId.h"
#include "Utilities.h"
#include "CPUDataContainer.h"
#include <chrono>
#include <cstdint>
#include <limits>

namespace Graphics
{
	/* Called by the engine to fill an owner that was loaded again, with the same functions that wrote it the first time. ownerId_ and partId_ are the ids Api.h functions take (a descriptor: set id and descriptor layout id). */
	using ResidencyLoadFunc = void (*)(PoolId ownerId_, PoolId partId_);

	/*
		The application's two functions for the files behind loadFromDisk, both only run when the application asks for them (Api.h saveDescriptorToDisk / changeDescriptorLayoutOnDisk), the engine never calls them.
		ResidencySaveFunc writes the data of the owner to its file (read it back from the entry, Api.h getDescriptorMemoryEntry). ResidencyChangeLayoutFunc rewrites its file for the current layout, so loadFromDisk finds the new variables in it next time.
	*/
	using ResidencySaveFunc = void (*)(PoolId ownerId_, PoolId partId_);
	using ResidencyChangeLayoutFunc = void (*)(PoolId ownerId_, PoolId partId_);

	/* What a draw or a ONESHOT pass does while something it uses is not in memory yet. Both modes start the load at the next frame start, a request is never served inside the frame that made it. */
	enum ResidencyLoadModeEnum
	{
		SKIP_UNTIL_LOADED, // the draw is skipped (a ONESHOT pass is held back) until the upload completed, the CPU never waits
		WAIT_UNTIL_LOADED // the CPU waits right after the frame's submit until the upload of the load completed, so the next frame can use it at once. Blocks the calling thread, meant for a renderer that runs on its own thread
	};

	/*
		The on demand loading settings of an owner kind (a descriptor set), filled field by field (start from `= {}`). Every setting applies to all parts of the owner, the load and unload itself is per part (a descriptor).

		absenceSecondsCount: the engine unloads a part (its memory entries are removed) once no draw or pass used it for this many seconds, and loads it again when one does. at least 1 (0 throws), UninitializedId = never unloaded automatically (still by hand, cleanup and scene switch).
		loadFromDisk: optional. Set it when the application has an asset pipeline: the engine calls it after a load to write the part again and keeps no CPU copy. Without it the engine gets a CPU copy of the bytes (as large as the part) when the part unloads and writes it back on a load: a part of an owner with an absenceSecondsCount has every write mirrored into it, otherwise the shadow or the CPU_SHARED memory is read at the unload, and a GPU_LOCAL part without a shadow can't unload by hand.
		saveToDisk, changeLayoutOnDisk: required when loadFromDisk is set (validate throws if one of the three is missing), and not allowed without it. The engine never calls them: a layout update doesn't call loadFromDisk, so the files keep the old layout (a load from them throws meanwhile) until the application calls the save and change layout functions itself once it knows the whole conversion succeeded. If the engine did it at the commit, a conversion that failed half way would already have rewritten the files.
		loadMode: see ResidencyLoadModeEnum.
	*/
	struct ResidencyParams
	{
		uint32_t absenceSecondsCount = 1;
		ResidencyLoadFunc loadFromDisk = nullptr;
		ResidencySaveFunc saveToDisk = nullptr;
		ResidencyChangeLayoutFunc changeLayoutOnDisk = nullptr;
		ResidencyLoadModeEnum loadMode = SKIP_UNTIL_LOADED;

		void validate(AllocatorTypeEnum allocatorType_) const; // throws std::runtime_error. A STATIC_ALLOCATOR owner can't unload (static entries can't be removed), so it has to set absenceSecondsCount to UninitializedId
	};

	constexpr uint64_t NeverUsedFrame = std::numeric_limits<uint64_t>::max(); // Residency::lastUsedFrame of a part no draw or pass used yet

	/*
		The load / unload state of one part (a descriptor). An unloaded part has no memory entry (its entry id is stale) and its binding points at a destroyed buffer, so nothing may read or write it.
		The CPU copy of the bytes is held in one place at a time: while the part is loaded an entry with a shadow (MemoryEntry::shadow) is the copy, so backup is empty and unloading moves the shadow in; an entry without a shadow has the writes mirrored into backup. See DescriptorSet::mirrorsWrites.
	*/
	struct Residency
	{
		bool loaded = true;
		bool loadRequested = false; // a draw or a ONESHOT pass used it while unloaded, DescriptorsManager::updateLoading loads it at the next frame start
		bool diskLayoutStale = false; // loadFromDisk set: a layout update changed the layout, the file still has the old one until changeLayoutOnDisk, so a load throws
		std::chrono::steady_clock::time_point lastUsedTime = std::chrono::steady_clock::now();
		uint64_t lastUsedFrame = NeverUsedFrame; // Renderer::frameNumber of the last draw or pass that used it, what PassesGraph::waitForUse decides on
		void markUsedFrame(); // stamps lastUsedFrame with the current Renderer::frameNumber
		CPUDataContainer backup; // written back by a load, kept only by an owner that can unload and has no loadFromDisk
	};
}
