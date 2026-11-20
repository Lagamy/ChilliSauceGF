#pragma once
#include "PoolId.h"
#include <vector>

namespace Graphics
{
	/*
		A pending edit of the descriptor layouts (bindings) of one DescriptorSetLayout, see beginSetLayoutUpdate / commitSetLayoutUpdate in Api.h.
		An added descriptor layout exists right away (so its id can be used to add its members) but is flagged DescriptorLayout::pending and ignored by the layout and its sets until the commit. A removed one is only listed here and stays until the commit.
		Only one update of a set layout can be open at a time.
	*/
	struct SetLayoutUpdate
	{
		PoolId setLayoutId;
		std::vector<PoolId> addedDescriptorLayoutIds;
		std::vector<PoolId> removedDescriptorLayoutIds;

		SetLayoutUpdate(PoolId setLayoutId_) : setLayoutId(setLayoutId_) {};
	};
}
