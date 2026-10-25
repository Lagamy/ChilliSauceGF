#pragma once
#include "DataContainerConcepts.h"
#include "MemoryEntryId.h"
#include "StagingData.h"
#include <cstdint>

/*
	GPUDataContainer owns no bytes and touches no Vulkan memory. It is a handle to a host-visible range that MemoryManager owns: write/read forward to MemoryManager, which
	finds the mapped memory and does the copy (and the invalidate for non-coherent readback memory). A Mesh or DescriptorSet therefore needs no CPU-side copy.
	Offsets are entry byte offsets.

	Lifetime differs by where the memory is:
	- GPU_ONLY entries: a per-update handle. stagingData is a range of an UPLOAD staging heap, and it exists only from the write until the batch that copies it completes,
	  after which the range is reused. Never store it (not a Mesh member). MemoryManager returns a fresh one for every update.
	  Upload staging is write-combined, so reading it is very slow. Reads are disabled at compile time for this variant (Readable == false).
	- Readbacks of GPU_ONLY entries: a handle valid from MemoryManager::getReadbackContainer until releaseReadback. stagingData is a range of a READBACK heap, so it can be read (Readable == true).
	  Same rule as the staging variant: never store it, the range is reused after release.
	- CPU_SHARED entries: a persistent handle. entryId is the entry whose mapped memory is edited, valid as long as the entry exists. It can be read.
*/
namespace Graphics
{
	template<bool Readable>
	struct GPUDataContainer
	{
		bool isShared = false; // true -> entryId is used (CPU_SHARED entry), false -> stagingData is used (staging or readback range).
		MemoryEntryId entryId;
		StagingData stagingData;
		uint32_t repeatCount = 1;
		/* The entry byte range write/read may touch. Checked under ENGINE_DEBUG, since a write outside it would corrupt a neighbouring range of the same memory. */
		uint64_t validFirstOffset = 0;
		uint64_t validSize = 0;

		void write(uint64_t offset_, const void* pSrc_, uint64_t size_);
		void read(uint64_t offset_, void* pDst_, uint64_t size_) const requires Readable;
	};

	using StagingDataContainer = GPUDataContainer<false>;
	using SharedDataContainer = GPUDataContainer<true>;
	using ReadbackDataContainer = GPUDataContainer<true>;
}
