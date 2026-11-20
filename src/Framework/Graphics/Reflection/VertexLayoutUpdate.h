#pragma once
#include "EntryCopyRequest.h"
#include "Layout.h"
#include "PoolId.h"
#include <vector>

namespace Graphics
{
	/*
		A pending edit of one vertex layout. beginVertexLayoutUpdate copies the layout, the add / remove functions of Api.h edit the copy (so the ids they return are the ones the layout has after the commit, and survivors keep theirs)
		and commitVertexLayoutUpdate writes it back. The live layout, the meshes and the pipelines stay as they were until then. Only one update of a layout can be open at a time.
	*/
	struct VertexLayoutUpdate
	{
		PoolId layoutId;
		ReflectionLayout staged;

		VertexLayoutUpdate(PoolId layoutId_, const ReflectionLayout& rLayout_) : layoutId(layoutId_), staged(rLayout_) {};
	};

	/* True if the bytes of a vertex entry made for the old layout are valid for the new one as they are: same stride and every variable of both layouts at its old offset (e.g. a variable that fit in the headroom was added). */
	bool vertexKeepsEntry(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_);

	/*
		The ranges that carry the bytes of verticesCount_ vertices from the old layout to the new one: one per vertex per variable that exists in both (same slot and generation), from where the old layout has it to where the new one has it.
		Neighbours that stay neighbours are merged by MemoryManager::copyEntry.
	*/
	std::vector<MemoryCopyRegion> makeVertexCopyRegions(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_, uint32_t verticesCount_);

	/*
		The ranges of the new entry that have to be zero, as regions with srcOffset 0 (a zero source) and the destination range in dstOffset: per vertex the bytes of the variables that are new (not in the old layout),
		or with wholeGaps_ every byte the surviving variables don't cover (new variables and the headroom), for an entry that is made from nothing and only gets the survivors copied into it.
	*/
	std::vector<MemoryCopyRegion> makeVertexZeroRegions(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_, uint32_t verticesCount_, bool wholeGaps_);
}
