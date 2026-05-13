#pragma once 

#include "HardcodedTriangleMesh.h"
#include "Engine.h"
#include <cstdint>

void gameCmdTransferCommands(VkCommandBuffer& rCmdBuffer_); 
void gameCmdGraphicsCommands(VkCommandBuffer& rCmdBuffer_); 
void gameCmdComputeCommands(VkCommandBuffer& rCmdBuffer_);	

// void editorCmdTransferCommnads(VkCommandBuffer& rCmdBuffer_); 
// void editorCmdGraphicsCommands(VkCommandBuffer& rCmdBuffer_);
// void editorCmdComputeCommands(VkCommandBuffer& rCmdBuffer_);

// void imGuiCmdTransferCommands(VkCommandBuffer& rCmdBuffer_); 
// void imGuiCmdGraphicsCommands(VkCommandBuffer& rCmdBuffer_);
// void imGutCmdComputeCommands(VkCommandBuffer& rCmdBuffer_);
