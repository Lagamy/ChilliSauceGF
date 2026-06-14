// Manages demo setup and connects it to the renderer.
#pragma once 

#include "Pool.h"
#include "Utilities.h"
#include "CmdBufferBlueprintsPack.h"
#include <vector>
#include <thread>

using demoFunc = std::function<void()>;
struct DemoManager {
	CmdBufferBlueprintsPack frameCmdBufferBlueprints; 
	CmdBufferBlueprintsPack oneShotCmdBufferBlueprints; 	

	uint32_t addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_);
	
	demoFunc loadDemo; 
	demoFunc submitToGPU; 
};
