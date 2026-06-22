#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include "Globals.h"
#include <vulkan/vulkan.h>


// Forward decloration 
namespace Graphics 
{
	/* Main API*/
	// Get 
	Instance& getInstance();
	Device& getMainDevice(); 
	Surface& getSurface();
	Swapchain& getSwapchain();
	GPUMemoryManager& getGPUMemoryManager(); 
	Semaphore& getSemaphore(PoolId semaphoreId_);
	Fence& getFence(PoolId fenceId_);
	CmdBufferBlueprintsPack& getCmdBufferBlueprints(CommandPoolTypeEnum poolType_); 
	const VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_, uint32_t id_); 
	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_);
	const UploadEntry& getUploadEntry(uint32_t id_, AllocatorTypeEnum allocatorType_, UploadTypeEnum uploadType_); 
	Buffer& getUploadHeapBuffer(AllocatorTypeEnum allocatorType_, UploadTypeEnum uploadType_);

	uint32_t& getCurrentImageIndex();
	FrameResources& getCurrentFrameResources(); 

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(); 
	RenderPass& getRenderPass();
	
	
	
	// Record 
	void recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_);
	void recordCurrentFrameCmdPools(); 

	// Reset 
	void resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_, Fence& rFinishSignalFence_);
	void resetCurrentFrameCmdPools();
	
	// Add
	PoolId addSemaphore(const char* name_);
	PoolId addFence(const char* name_, VkFenceCreateFlags flags_);
	uint32_t addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_);
	uint32_t addUpload(const char* name_, AllocatorTypeEnum allocatorType_, const void* data_, VkDeviceSize size_, UploadTypeEnum uploadType_);
};
