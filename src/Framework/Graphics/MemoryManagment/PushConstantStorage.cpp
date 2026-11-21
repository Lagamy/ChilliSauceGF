#include "PushConstantStorage.h"
#include "Api.h"
#include <format>
#include <stdexcept>

namespace Graphics
{
void PushConstantStorage::setup()
{
	this->maxSize = getMainDevice().properties.limits.maxPushConstantsSize;
}

void PushConstantStorage::place(std::vector<PushConstantUse>& rUses_)
{
	uint64_t cursor = 0;
	std::string layoutsInfo;
	for(PushConstantUse& rUse : rUses_)
	{
		PushConstantLayout& rLayout = getPipelinesManager().pushConstantLayouts[rUse.layoutId];
		uint64_t size = rLayout.makeShape().blockSize;
		rUse.offset = static_cast<uint32_t>(alignUp(cursor, rLayout.getAlignment()));
		cursor = rUse.offset + size;
		layoutsInfo += std::format("\n  {}: {} bytes (at {})", rLayout.name, size, rUse.offset);
	}

	if(cursor > this->maxSize)
	{
		throw std::runtime_error(std::format("Push Constant Storage: the push constant layouts of a pipeline need {} bytes together:{}\nThe device allows {} bytes (maxPushConstantsSize).", cursor, layoutsInfo, this->maxSize));
	}
}
}
