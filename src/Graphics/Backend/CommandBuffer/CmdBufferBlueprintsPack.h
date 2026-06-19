#pragma once
#include <vector>
#include "CommandBufferBlueprint.h"

namespace Graphics
{
struct CmdBufferBlueprintsPack { 
	std::vector<CommandBufferBlueprint> graphics;
	std::vector<CommandBufferBlueprint> transfer;
	std::vector<CommandBufferBlueprint> compute;
};
}
