#pragma once
#include "PushConstantLayout.h"
#include <cstdint>
#include <vector>

namespace Graphics
{
/*
	The push constant budget of the device (limits.maxPushConstantsSize, at least 128 bytes) and the placement inside it. No memory is allocated: push constants live in the command buffer.
	A pipeline layout has one push constant range per shader stage and the ranges of a pipeline are placed one after another, so the layouts attached to a pipeline have to fit the budget together.
	The sizes are only known once the user stopped adding layouts to the pipeline, so the offsets are worked out when the pipeline layout is created (PipelineLayout::create), not when a layout is attached.
*/
struct PushConstantStorage
{
	uint32_t maxSize = 0;

	void setup(); // after the device exists

	/*
		Sets the offset of every use: in the order of rUses_, each at the next multiple of the alignment of its layout (PushConstantLayout::getAlignment).
		Throws when the total is more than maxSize, naming every layout of the uses with its size and the device's maximum.
	*/
	void place(std::vector<PushConstantUse>& rUses_);
};
}
