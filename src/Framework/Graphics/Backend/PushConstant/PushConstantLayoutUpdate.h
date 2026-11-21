#pragma once
#include "PushConstantLayout.h"
#include "PoolId.h"

namespace Graphics
{
	/*
		A pending edit of the members of one PushConstantLayout. beginPushConstantLayoutUpdate copies the layout, the add / remove functions of Api.h edit the copy (so the ids they return are the ones the layout has after the commit) and commitPushConstantLayoutUpdate writes the copy back.
		The live layout stays as it was until then. Only one update of a layout can be open at a time.
	*/
	struct PushConstantLayoutUpdate
	{
		PoolId layoutId;
		PushConstantLayout staged;

		PushConstantLayoutUpdate(PoolId layoutId_, const PushConstantLayout& rLayout_) : layoutId(layoutId_), staged(rLayout_) {};
	};
}
