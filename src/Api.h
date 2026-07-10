#pragma once 
#include "Layout.h"
#include "PoolId.h"
#include "UploadId.h"
#include "Utilities.h"
#include "Globals.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>


// Forward decloration 
namespace Graphics 
{
	/* Main API*/
	// Get 
	GLFWwindow* getWindowPointer(); 
	Instance& getInstance();
	Device& getMainDevice(); 
	Surface& getSurface();
	Swapchain& getSwapchain();
	DemoManager& getDemoManager();
	GPUMemoryManager& getGPUMemoryManager(); 
	Semaphore& getSemaphore(PoolId semaphoreId_);
	Fence& getFence(PoolId fenceId_);
	Shader& getShader(PoolId shaderId_); 
	CmdBufferBlueprintsPack& getCmdBufferBlueprints(CommandPoolTypeEnum poolType_); 
	uint32_t getBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum bufferType_); 
	const VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_, uint32_t id_); 
	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_);
	const UploadEntry& getUploadEntry(UploadId id_); 
	Mesh& getMesh(PoolId meshId_); 
	Buffer& getGPUBuffer(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	uint32_t getGPUBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	uint32_t getUploadStartingByteInGPUHeap(UploadId id_); 

	uint32_t& getCurrentImageIndex();
	FrameResources& getCurrentFrameResources(); 

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(); 
	RenderPass& getRenderPass();
	
	ReflectionLayout& getVerticeLayout(PoolId layoutId_);


	// Set
	void setFramesAtFlightCount(uint32_t count_); 

	// Record 
	void recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_);
	void recordCurrentFrameCmdPools(); 

	// Reset 
	void resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_, Fence& rFinishSignalFence_);
	void resetCurrentFrameCmdPools();
	
	// Add
	PoolId addSemaphore(const char* name_);
	PoolId addFence(const char* name_, VkFenceCreateFlags flags_);
	PoolId addShader(const char* name_, const char* path_); 
	PoolId addMesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_); 
	uint32_t addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_);
	UploadId addUpload(const char* name_, AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_, const void* data_, VkDeviceSize size_);

	PoolId addVerticeLayout(const char* name_);
	PoolId addMemberToVerticeLayout(PoolId verticeLayoutId_, const char* name_, DataTypeEnum dataType_);
	
	// Remove 
	void removeShader(PoolId id_);
};