#include "VertexLayoutUpdate.h"
#include <algorithm>

namespace Graphics
{
	/* The variables alive in both layouts with the same slot and generation, as (old, new) pairs. */
	static std::vector<std::pair<const VariableBlueprint*, const VariableBlueprint*>> getSurvivingVariables(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_)
	{
		std::vector<std::pair<const VariableBlueprint*, const VariableBlueprint*>> survivors;
		const PoolMap<VariableBlueprint>& rOld = rOldLayout_.variableBlueprints;
		const PoolMap<VariableBlueprint>& rNew = rNewLayout_.variableBlueprints;
		for(uint32_t slot = 0; slot < rNew.objects.size(); slot++)
		{
			if(rNew.alive[slot] && slot < rOld.objects.size() && rOld.alive[slot] && rOld.generation[slot] == rNew.generation[slot])
			{
				survivors.emplace_back(&rOld.objects[slot], &rNew.objects[slot]);
			}
		}
		return survivors;
	}

	bool vertexKeepsEntry(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_)
	{
		if(rOldLayout_.size != rNewLayout_.size)
		{
			return false;
		}
		for(const auto& [pOld, pNew] : getSurvivingVariables(rOldLayout_, rNewLayout_))
		{
			if(pOld->firstByteId != pNew->firstByteId)
			{
				return false;
			}
		}
		return true;
	}

	std::vector<MemoryCopyRegion> makeVertexCopyRegions(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_, uint32_t verticesCount_)
	{
		std::vector<MemoryCopyRegion> regions;
		auto survivors = getSurvivingVariables(rOldLayout_, rNewLayout_);
		regions.reserve(survivors.size() * verticesCount_);
		for(uint32_t verticeId = 0; verticeId < verticesCount_; verticeId++)
		{
			for(const auto& [pOld, pNew] : survivors)
			{
				regions.push_back({verticeId * rOldLayout_.size + pOld->firstByteId, verticeId * rNewLayout_.size + pNew->firstByteId, pNew->size});
			}
		}
		return regions;
	}

	std::vector<MemoryCopyRegion> makeVertexZeroRegions(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_, uint32_t verticesCount_, bool wholeGaps_)
	{
		auto survivors = getSurvivingVariables(rOldLayout_, rNewLayout_);

		/* The ranges inside one vertex, in the order of the bytes. */
		std::vector<std::pair<uint64_t, uint64_t>> perVertex;
		const PoolMap<VariableBlueprint>& rNew = rNewLayout_.variableBlueprints;
		if(wholeGaps_)
		{
			std::vector<std::pair<uint64_t, uint64_t>> covered;
			for(const auto& [pOld, pNew] : survivors)
			{
				covered.emplace_back(pNew->firstByteId, pNew->size);
			}
			std::sort(covered.begin(), covered.end());
			uint64_t cursor = 0;
			for(const auto& [first, size] : covered)
			{
				if(first > cursor)
				{
					perVertex.emplace_back(cursor, first - cursor);
				}
				cursor = first + size;
			}
			if(cursor < rNewLayout_.size)
			{
				perVertex.emplace_back(cursor, rNewLayout_.size - cursor);
			}
		}
		else
		{
			for(uint32_t slot = 0; slot < rNew.objects.size(); slot++)
			{
				bool isNew = rNew.alive[slot];
				for(const auto& [pOld, pNew] : survivors)
				{
					isNew = isNew && pNew != &rNew.objects[slot];
				}
				if(isNew)
				{
					perVertex.emplace_back(rNew.objects[slot].firstByteId, rNew.objects[slot].size);
				}
			}
		}

		std::vector<MemoryCopyRegion> regions;
		regions.reserve(perVertex.size() * verticesCount_);
		for(uint32_t verticeId = 0; verticeId < verticesCount_; verticeId++)
		{
			for(const auto& [first, size] : perVertex)
			{
				regions.push_back({0, verticeId * rNewLayout_.size + first, size});
			}
		}
		return regions;
	}
}
