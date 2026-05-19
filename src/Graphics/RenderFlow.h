// Swapable class that decides render flow 
#pragma once 

#include "Utilities.h"
#include "CommandBufferBlueprint.h"
#include <vector>
#include <thread>


// Every member here will create a prefab usable in other scenes 
// Can be applied to a Scene. Saved as a binary in a folder, with other resources being in children folders. 
struct RenderFlow { 

	std::vector<CommandBufferBlueprint> graphicsCmdBufferBlueprints;
	std::vector<CommandBufferBlueprint> transferCmdBufferBlueprints;
	std::vector<CommandBufferBlueprint> computeCmdBufferBlueprints;

	void addCmdBufferBlueprint(QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_);
};
