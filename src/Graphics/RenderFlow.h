// Swapable class that decides render flow 
#pragma once 

#include "Utilities.h"
#include "CmdBufferBlueprintsPack.h"
#include <vector>
#include <thread>


// Every member here will create a prefab usable in other scenes 
// Can be applied to a Scene. Saved as a binary in a folder, with other resources being in children folders. 
struct RenderFlow { 
	CmdBufferBlueprintsPack frameCmdBufferBlueprints; 
	CmdBufferBlueprintsPack oneShotCmdBufferBlueprints; 	

	void addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_);
};
