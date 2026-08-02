#pragma once 
#include "Layout.h"
#include "PassesGraph.h"
#include "PoolId.h"
#include "PassId.h"
#include "Pass.h"
#include "UploadId.h"
#include "Utilities.h"
#include "Globals.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


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
	GPUScene& getGPUSceneManager();
	MemoryManager& getMemoryManager(); 
	PassesGraph& getPassesManager(); 
	
	VkSemaphore& getSemaphore(PoolId semaphoreId_);
	VkFence& getFence(PoolId fenceId_);
	VkQueue& getQueue(uint8_t id_); 
	Shader& getShader(PoolId shaderId_); 
	uint32_t getBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum bufferType_); 
	VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CmdLifetimeEnum poolType_, uint32_t id_); 
	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CmdLifetimeEnum poolType_);
	const UploadEntry& getUploadEntry(UploadId id_); 
	bool isUploadInGPU(UploadId uploadId_);
	Mesh& getMesh(PoolId meshId_); 
	Buffer& getGPUBuffer(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	uint32_t getGPUBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	uint32_t getUploadStartingByteInGPUHeap(UploadId id_);
	Pass& getPass(PassId passId_);
	PoolId getCurrentSwapchainImageUseFinishedSemaphore(); 
	PoolId getCurrentFrameImageIsAcquiredSemaphore(); 
	PoolId getCurrentFrameAvailableFence();


	uint32_t& getCurrentImageIndex();
	uint32_t& getCurrentFrameIndex(); 
	FrameResources& getCurrentFrameResources(); 
	Image& getCurrentSwapchainImage(); 

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(PoolId layoutId_); 
	RenderPass& getPresentationRenderPass();
	
	ReflectionLayout& getVerticeLayout(PoolId layoutId_);


	// Set
	void setFramesAtFlightCount(uint32_t count_); 

	void resetFences(std::span<VkFence> fences_);
	void resetFence(PoolId fenceId_); 
	bool wasFenceSignaled(PoolId fenceId_); 
	
	// Record 
	void recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_);
	void recordCurrentFrameCmdPools(); 

	// Reset 
	void resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_, Fence& rFinishSignalFence_);
	void resetCurrentFrameCmdPools();
	
	// Add
	PoolId addSemaphore();
	PoolId addFence(bool createSignaled_);
	PoolId addShader(const char* name_, const char* path_); 
	PoolId addMesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_); 
	UploadId addUpload(const char* name_, AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_, const void* data_, VkDeviceSize size_);
	PoolId addVerticeLayout(const char* name_);
	PoolId addMemberToVerticeLayout(PoolId verticeLayoutId_, const char* name_, DataTypeEnum dataType_);
	PoolId addGraphicsPipelineLayout(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_); 
	void createAllPipelines();
	void destroyAllPipelines();

	PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc signalFenceRetrivalFunc_); 
    uint32_t addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_);
	void addWaitSemaphoreToTask(PassId passsId_, uint32_t taskId_, PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_); 
	void addSignalSemaphoreToTask(PassId passsId_, uint32_t taskId_, PoolId signalSemaphoreId_); 
	void addDynamicWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_); 
    void addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_);

	// Enable/Disable Pass 
	void enablePass(PassId passId_); 
    void disableFramePass(PassId passId_); // Since oneshot - self disables
			
	// Remove 
	void removeShader(PoolId id_);
	
	// Commands Recording
	void beginCMDsRecording(VkCommandBuffer& cmdBuffer_);
	void bindGraphicsPipeline(PoolId graphicsPipelineId_, VkCommandBuffer& cmdBuffer_); 
	void drawMeshIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer& cmdBuffer_);
	void endCMDsRecording(VkCommandBuffer& cmdBuffer_); 

	// Misc/Internal 
	void setViewportAndScissors(VkCommandBuffer& cmdBuffer_); 
	void presentToScreen(); 
};