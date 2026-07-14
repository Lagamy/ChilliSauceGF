#pragma once 
#include "Layout.h"
#include "PoolId.h"
#include "SubmissionBatch.h"
#include "SubmissionBatchId.h"
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
	DemoManager& getDemoManager();
	GPUMemoryManager& getGPUMemoryManager(); 
	VkSemaphore& getSemaphore(PoolId semaphoreId_);
	VkFence& getFence(PoolId fenceId_);
	Shader& getShader(PoolId shaderId_); 
	CmdBufferBlueprintsPack& getCmdBufferBlueprints(CommandPoolTypeEnum poolType_); 
	uint32_t getBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum bufferType_); 
	VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_, uint32_t id_); 
	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_);
	const UploadEntry& getUploadEntry(UploadId id_); 
	Mesh& getMesh(PoolId meshId_); 
	Buffer& getGPUBuffer(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	uint32_t getGPUBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	uint32_t getUploadStartingByteInGPUHeap(UploadId id_);
	SubmissionBatch& getSubmissionBatch(SubmissionBatchId batchId_);

	uint32_t& getCurrentImageIndex();
	FrameResources& getCurrentFrameResources(); 
	Image& getCurrentSwapchainImage(); 

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(PoolId graphicsPipelineId_); 
	RenderPass& getPresentationRenderPass();
	
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
	PoolId addGraphicsPipeline(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_); 
	SubmissionBatchId addSubmissionBatch(const char* name_, QueueFamilyEnum familyEnum_); 
	PoolId addSubmission(const char* name_, SubmissionBatchId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_);
    void addWaitSemaphoreToSubmission(SubmissionBatchId batchId_, PoolId submitionId_, VkSemaphore waitSemaphore_, VkPipelineStageFlags pipelineStage_);  
    void addSignalSemaphoreToSubmission(SubmissionBatchId batchId_, PoolId submitionId_, VkSemaphore signalSemaphore_);
   
	// Submition 
	void submitToGraphicsQueue(SubmissionBatchId batchId_, VkFence signalFence_);
    void submitToTransferQueue(SubmissionBatchId batchId_, VkFence signalFence_); 
    void submitToComputeQueue(SubmissionBatchId batchId_, VkFence signalFence_); 


	// Remove 
	void removeShader(PoolId id_);
	
	// Commands Recording
	void beginCMDsRecording(VkCommandBuffer& cmdBuffer_);
	void bindGraphicsPipeline(PoolId graphicsPipelineId_, VkCommandBuffer& cmdBuffer_); 
	void bindMesh(PoolId meshId_, VkCommandBuffer& cmdBuffer_);
	void drawIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer& cmdBuffer_); 
	void endCMDsRecording(VkCommandBuffer& cmdBuffer_); 

	// Misc 
	void setViewportAndScissors(VkCommandBuffer& cmdBuffer_); 
	void presentToScreen(); 
};