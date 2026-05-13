// Swapable class that decides render flow 
#pragma once 
#include "CommandBufferBlueprint.h"
#include <vector>
#include <thread>


// Every member here will create a prefab usable in other scenes 
// Can be applied to a Scene. Saved as a binary in a folder, with other resources being in children folders. 
using func = void(*)(); 
struct RenderFlow { 

	CommandBufferBlueprint graphicsCmdBufferBlueprint;
	CommandBufferBlueprint transferCmdBufferBlueprint;
	CommandBufferBlueprint computeCmdBufferBlueprint;

	void initPrimaryCmdBufferBlueprint(CmdBufferTypeEnum cmdBufferType_, Cmd commandsToRecord_);

	RenderFlow(); 
	~RenderFlow(); 
};
