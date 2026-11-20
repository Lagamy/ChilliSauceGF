#pragma once
#include "DescriptorLayout.h"
#include "DescriptorShape.h"
#include "EntryCopyRequest.h"
#include "PoolId.h"
#include <vector>

namespace Graphics
{
	/*
		A pending edit of the members of one DescriptorLayout (the block of one binding). beginDescriptorLayoutUpdate copies the layout, the add / remove functions of Api.h edit the copy (so the ids they return are the ones the layout has after the commit) and
		commitDescriptorLayoutUpdate writes the copy back. The live layout, its shape and the data of its descriptors stay as they were until then, so writes through the old ids keep working while the update is open.
		Only one update of a layout can be open at a time.
	*/
	struct DescriptorLayoutUpdate
	{
		PoolId setLayoutId;
		PoolId descriptorLayoutId;
		DescriptorLayout staged;

		DescriptorLayoutUpdate(PoolId setLayoutId_, PoolId descriptorLayoutId_, const DescriptorLayout& rLayout_) : setLayoutId(setLayoutId_), descriptorLayoutId(descriptorLayoutId_), staged(rLayout_) {};
	};

	/*
		The bytes of a descriptor after its block changed: every variable that exists in both layouts (same slot and generation) is copied from where the old shape had it to where the new shape has it, for every dynamic unit and struct element. The rest (new variables) is zero.
		rOldBytes_ is the whole old entry, e.g. Residency::backup. Returns the whole new entry.
	*/
	std::vector<char> remapDescriptorBytes(const std::vector<char>& rOldBytes_, const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_);

	/*
		The GPU side twin of remapDescriptorBytes: one region per variable that exists in both layouts (per dynamic unit and struct element), srcOffset in the old entry, dstOffset in the new one. MemoryManager::copyEntry merges the ones that touch.
	*/
	std::vector<MemoryCopyRegion> makeDescriptorCopyRegions(const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_);

	/*
		Every byte of the new entry the surviving variables don't cover (new variables and padding) as regions with srcOffset 0 and dstOffset set to the range, for MemoryManager::zeroEntryRanges.
	*/
	std::vector<MemoryCopyRegion> makeDescriptorZeroRegions(const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_);

	/*
		True if the entry of the old shape can be used for the new one as it is: same entry size and block size (so the binding range is the same) and every variable of both layouts is where it was.
		That is the case when members were only added (or removed) and fit in the headroom. The new variables are then whatever the entry held there (zero in the CPU backup).
	*/
	bool descriptorKeepsEntry(const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_);
}
