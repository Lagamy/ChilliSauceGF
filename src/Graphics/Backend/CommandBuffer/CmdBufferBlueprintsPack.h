#pragma once
#include <vector>
#include "CommandBufferBlueprint.h"

struct CmdBufferBlueprintsPack { 
	std::vector<CommandBufferBlueprint> graphics;
	std::vector<CommandBufferBlueprint> transfer;
	std::vector<CommandBufferBlueprint> compute;
};
