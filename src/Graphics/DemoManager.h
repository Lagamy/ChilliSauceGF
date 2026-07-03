// Manages demo setup and connects it to the renderer.
#pragma once 

#include "Utilities.h"
#include "CmdBufferBlueprintsPack.h"
#include "Triangle.h"
#include <vector>
#include <thread>

namespace Graphics
{
using demoFunc = std::function<void()>;
struct DemoManager {
	CmdBufferBlueprintsPack frameCmdBufferBlueprints; 
	CmdBufferBlueprintsPack oneShotCmdBufferBlueprints;
	

	uint32_t addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_);
	
	demoFunc loadDemo; 
	demoFunc submitToGPU; 
	// You can just include demos there, and comment out demos that aren't active
	Triangle triangle; 
};
}
