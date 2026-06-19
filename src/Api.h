#pragma once 
#include "Globals.h"

namespace Graphics 
{
/* Main API*/
// Get 
Device& getMainDevice(); 
Swapchain& getSwapchain();
// For now i only need 1 of each
GraphicsPipeline& getGraphicsPipeline(); 
RenderPass& getRenderPass(); 
PoolId getSemaphore(const char* name_);
PoolId getFence(const char* name_);
	
const VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_, uint32_t id_); 
const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_);
	
	
// Record 
void recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_);
void recordCurrentFrameCmdPools(); 

// Reset 
void resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_, Fence& rFinishSignalFence_);
void resetCurrentFrameCmdPools();
	
// Add
Semaphore& getSemaphore(PoolId semaphoreId_);
Fence& getFence(PoolId fenceId_);
uint32_t addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_);
uint32_t addUpload(const char* name_, AllocatorTypeEnum allocatorType_, const void* data_, VkDeviceSize size_, UploadTypeEnum uploadType_);
};
