#include "DescriptorLayoutUpdate.h"
#include <algorithm>
#include <cstring>

namespace Graphics
{
	/* The element picked by flatId_ of a repeated struct, last layer fastest: the sum of id x stride over its layers. */
	static uint64_t getElementOffset(const RepeatShape& rShape_, uint32_t flatId_)
	{
		uint64_t offset = 0;
		for(size_t layerId = rShape_.layers.size(); layerId-- > 0;)
		{
			offset += (flatId_ % rShape_.layers[layerId].count) * rShape_.layers[layerId].stride;
			flatId_ /= rShape_.layers[layerId].count;
		}
		return offset;
	}

	/*
		Calls fn_(from, to, size) for every variable that exists in both layouts (same slot and generation in its member's layout, in a member with the same slot and generation), for every dynamic unit and struct element:
		from is its first byte in the old entry, to in the new one.
	*/
	template<typename F>
	static void forEachSurvivingVariable(const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_, F fn_)
	{
		uint32_t unitCount = rNewShape_.isDynamic() ? rNewShape_.dynamicLayer.count : 1;
		uint64_t oldUnitStride = rOldShape_.isDynamic() ? rOldShape_.dynamicLayer.stride : 0;
		uint64_t newUnitStride = rNewShape_.isDynamic() ? rNewShape_.dynamicLayer.stride : 0;

		for(uint32_t memberSlot = 0; memberSlot < rNewLayout_.members.objects.size(); memberSlot++)
		{
			if(!rNewLayout_.members.alive[memberSlot] || memberSlot >= rOldLayout_.members.objects.size() || !rOldLayout_.members.alive[memberSlot] || rOldLayout_.members.generation[memberSlot] != rNewLayout_.members.generation[memberSlot])
			{
				continue;
			}

			const MemberBlueprint& rOldMember = rOldLayout_.members.objects[memberSlot];
			const MemberBlueprint& rNewMember = rNewLayout_.members.objects[memberSlot];
			uint32_t elementCount = 1;
			if(rNewMember.kind == STRUCT_MEMBER)
			{
				for(uint32_t count : rNewMember.repeatCountPerDimension)
				{
					elementCount *= count;
				}
			}

			for(uint32_t unitId = 0; unitId < unitCount; unitId++)
			{
				for(uint32_t elementId = 0; elementId < elementCount; elementId++)
				{
					uint64_t oldBase = unitId * oldUnitStride + rOldShape_.placements[memberSlot].firstByteId;
					uint64_t newBase = unitId * newUnitStride + rNewShape_.placements[memberSlot].firstByteId;
					if(rNewMember.kind == STRUCT_MEMBER)
					{
						oldBase += getElementOffset(rOldShape_.placements[memberSlot].shape, elementId);
						newBase += getElementOffset(rNewShape_.placements[memberSlot].shape, elementId);
					}

					const PoolMap<VariableBlueprint>& rOldVariables = rOldMember.layout.variableBlueprints;
					const PoolMap<VariableBlueprint>& rNewVariables = rNewMember.layout.variableBlueprints;
					for(uint32_t variableSlot = 0; variableSlot < rNewVariables.objects.size(); variableSlot++)
					{
						if(!rNewVariables.alive[variableSlot] || variableSlot >= rOldVariables.objects.size() || !rOldVariables.alive[variableSlot] || rOldVariables.generation[variableSlot] != rNewVariables.generation[variableSlot])
						{
							continue;
						}

						const VariableBlueprint& rOldVariable = rOldVariables.objects[variableSlot];
						const VariableBlueprint& rNewVariable = rNewVariables.objects[variableSlot];
						fn_(oldBase + rOldVariable.firstByteId, newBase + rNewVariable.firstByteId, rNewVariable.size);
					}
				}
			}
		}
	}

	std::vector<char> remapDescriptorBytes(const std::vector<char>& rOldBytes_, const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_)
	{
		std::vector<char> newBytes(rNewShape_.getSize(), 0);
		forEachSurvivingVariable(rOldLayout_, rOldShape_, rNewLayout_, rNewShape_, [&](uint64_t from_, uint64_t to_, uint64_t size_)
		{
			if(from_ + size_ <= rOldBytes_.size() && to_ + size_ <= newBytes.size())
			{
				std::memcpy(&newBytes[to_], &rOldBytes_[from_], size_);
			}
		});
		return newBytes;
	}

	std::vector<MemoryCopyRegion> makeDescriptorCopyRegions(const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_)
	{
		std::vector<MemoryCopyRegion> regions;
		forEachSurvivingVariable(rOldLayout_, rOldShape_, rNewLayout_, rNewShape_, [&](uint64_t from_, uint64_t to_, uint64_t size_)
		{
			regions.push_back({from_, to_, size_});
		});
		return regions;
	}

	std::vector<MemoryCopyRegion> makeDescriptorZeroRegions(const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_)
	{
		std::vector<std::pair<uint64_t, uint64_t>> covered;
		forEachSurvivingVariable(rOldLayout_, rOldShape_, rNewLayout_, rNewShape_, [&](uint64_t, uint64_t to_, uint64_t size_)
		{
			covered.emplace_back(to_, size_);
		});
		std::sort(covered.begin(), covered.end());

		std::vector<MemoryCopyRegion> regions;
		uint64_t cursor = 0;
		for(const auto& [first, size] : covered)
		{
			if(first > cursor)
			{
				regions.push_back({0, cursor, first - cursor});
			}
			cursor = first + size;
		}
		if(cursor < rNewShape_.getSize())
		{
			regions.push_back({0, cursor, rNewShape_.getSize() - cursor});
		}
		return regions;
	}

	bool descriptorKeepsEntry(const DescriptorLayout& rOldLayout_, const DescriptorShape& rOldShape_, const DescriptorLayout& rNewLayout_, const DescriptorShape& rNewShape_)
	{
		if(rOldShape_.getSize() != rNewShape_.getSize() || rOldShape_.blockSize != rNewShape_.blockSize)
		{
			return false;
		}
		bool keeps = true;
		forEachSurvivingVariable(rOldLayout_, rOldShape_, rNewLayout_, rNewShape_, [&](uint64_t from_, uint64_t to_, uint64_t)
		{
			keeps = keeps && from_ == to_;
		});
		return keeps;
	}
}
