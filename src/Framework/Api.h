#pragma once 
#include "DescriptorsManager.h"
#include "Layout.h"
#include "PassesGraph.h"
#include "PipelinesManager.h"
#include "PoolId.h"
#include "PassId.h"
#include "Pass.h"
#include "MemoryEntryId.h"
#include "MemoryEntryCreateParams.h"
#include "ProjectManager.h"
#include "Utilities.h"
#include "Globals.h"
#include "GLFW/glfw3.h"
#include <span>
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
	void switchDevice(uint32_t id_); // rebuilds the renderer on suitableDevices[id_]. Not while recording or inside draw
	Surface& getSurface();
	Swapchain& getSwapchain();
	GPUSceneManager& getGPUSceneManager();
	MemoryManager& getMemoryManager(); 
	PassesGraph& getPassesManager(); 
	PipelinesManager& getPipelinesManager(); 
	DescriptorsManager& getDescriptorsManager(); 
	ReflectionSystem& getReflectionSystem(); 
	
	VkSemaphore& getSemaphore(PoolId semaphoreId_);
	VkFence& getFence(PoolId fenceId_);
	const VkFence& getFenceConst(PoolId fenceId_);
	VkQueue& getQueue(uint8_t id_); 
	Shader& getShader(PoolId shaderId_); 
	uint32_t getBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum bufferType_); 
	VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CmdLifetimeEnum poolType_, uint32_t id_); 
	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CmdLifetimeEnum poolType_);
	const MemoryEntry& getMemoryEntry(MemoryEntryId id_); 
	bool isUploadInGPU(MemoryEntryId uploadId_);
	Mesh& getMesh(PoolId meshId_); 
	Buffer& getGPUBuffer(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	uint32_t getGPUBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_);
	Pass& getPass(PassId passId_);
	PoolId getCurrentSwapchainImageUseFinishedSemaphore(); 
	PoolId getCurrentFrameImageIsAcquiredSemaphore(); 
	uint64_t getIndividualUpdateMemBlockSize(); 


	uint32_t& getCurrentImageIndex();
	uint32_t& getCurrentFrameIndex(); 
	FrameResources& getCurrentFrameResources(); 
	Image& getCurrentSwapchainImage(); 

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(PoolId layoutId_); 
	RenderPass& getPresentationRenderPass();
	ReflectionLayout& getVerticeLayout(PoolId layoutId_);
	DescriptorSet& getDescriptorSet(PoolId setId_);
	DescriptorSetLayout& getDescriptorSetLayout(PoolId layoutId_);


	// Set
	void setEnvironmentSetupFunction(ProjectFunc setupEnvironmetFunc_);
	void setFramesAtFlightCount(uint32_t count_); 
	void setIndividualUpdateMemBlockSize(uint64_t value_); 

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
	void addDeviceExtension(const char* name_); 
	PoolId addSemaphore();
	PoolId addFence(bool createSignaled_);
	PoolId addShader(const char* name_, const char* path_); 
	void addDescriptorsToShader(PoolId shaderId_, uint32_t setIndex_, PoolId setLayoutId_, const std::vector<PoolId>& descriptorLayoutIds_); // declares that the shader reads the descriptors of those descriptor layouts in the set (derived from setLayoutId_) bound at setIndex_. Graphics pipelines build their pipeline layout from these, so declare before adding the pipeline. drawMeshIndexed waits for the data of exactly these. Shaders without descriptors are fine. The declaration is what lets drawMeshIndexed check these descriptors and skip the draw until their first data is copied, so every descriptor the shader reads has to be declared: nothing checks the SPIR-V, an undeclared one can be read before its data was copied
	PoolId addMesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_); 
	MemoryEntryId addAndUploadMemoryEntry(const MemoryEntryCreateParams& rParams_); // params.data and params.size describe the bytes to upload

	MemoryEntryId allocateMemoryEntry(MemoryEntryCreateParams params_, PoolId layoutId_, uint32_t repeatCount_); // the size comes from the layout, so params.size has to stay 0

	MemoryEntryId allocateMemoryEntryRaw(const MemoryEntryCreateParams& rParams_);
	void updateMemoryEntry(MemoryEntryId uploadId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);
	ReadbackId requestMemoryEntryReadback(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_);
	bool isReadbackReady(ReadbackId readbackId_);
	ReadbackDataContainer mapReadback(ReadbackId readbackId_);
	void releaseReadback(ReadbackId readbackId_);

	PoolId addVerticeLayout(const char* name_);
	PoolId addVariableToVerticeLayout(PoolId verticeLayoutId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {});
	PoolId addVariableToVerticeLayout(PoolId verticeLayoutId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
	PoolId addDescriptorSetLayout(const char* name_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_ = DescriptorSetsPerPool); // the layout adds Descriptor Pools by itself as sets are derived, each one sized for setsPerPool_ sets
	PoolId getLayoutId(const char* layoutName_); // id of the Descriptor Set Layout added with that name, throws if there is none
	PoolId addBufferDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_); // type_: UNIFORM_DESCRIPTOR or STORAGE_DESCRIPTOR, a set can mix both. The members of the block are added with addMemberToDescriptorLayout / addStructMemberToDescriptorLayout + addVariableToDescriptorLayout
	PoolId addBufferDynamicDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_, uint32_t dynamicCount_); // same, but the whole block is repeated dynamicCount_ times and one of them is picked per bind (bindDescriptorSet dynamicUnitIds_) and written with setDynamicDescriptorMember...
	PoolId addImageDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_); // TEXTURE_DESCRIPTOR / SAMPLER_DESCRIPTOR. Not supported yet, throws
	PoolId addMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // A variable directly in the block (mat4 model;). Returns the variable id the setters take
	PoolId addStructMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, const std::vector<uint32_t>& repeatCountPerDimension_); // A struct in the block repeated per layer, outermost first ({2, 3} = states[2][3], {1} = a single struct). Returns the struct member id, its variables are added with addVariableToDescriptorLayout
	PoolId addVariableToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // A variable of a struct member. Returns the variable id the setters take
	PoolId addVariableToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
	PoolId addMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
	PoolId addDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_);
	void removeDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_);
	void removeDescriptorSet(PoolId setId_);
	void removeDescriptorSetLayout(PoolId layoutId_);
	PoolId addGraphicsPipelineLayout(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_); 
	void addPageForStagingReadbacks(uint32_t upperBoundForRangeSize_, StorageUnitEnum upperBoundUnit_, uint32_t heapSize_, StorageUnitEnum heapSizeUnit_, uint32_t maxDormantHeaps_);
	void addPageForStagingUploads(uint32_t upperBoundForRangeSize_, StorageUnitEnum upperBoundUnit_, uint32_t heapSize_, StorageUnitEnum heapSizeUnit_, uint32_t maxDormantHeaps_);
	void addPageForDynamicMemoryEntries(MemoryVisabilityEnum memoryVisability_, uint32_t upperBoundForEntrySize_, StorageUnitEnum upperBoundUnit_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_);

	void createAllPipelines();
	void destroyAllPipelines();

	PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc signalFenceRetrivalFunc_); 
    uint32_t addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_);
	void addWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_); 
	void addSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, PoolId signalSemaphoreId_); 
	void addDynamicWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_); 
    void addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_);

	// Enable/Disable Pass 
	void enablePass(PassId passId_); 
    void disableFramePass(PassId passId_); // Since oneshot - self disables
			
	// Remove 
	void removeShader(PoolId id_);
	void removeFence(PoolId id_); 
	void removeSemaphore(PoolId id_);
	void removeUpload(MemoryEntryId uploadId_);
	
	// Commands Recording
	void beginCMDsRecording(VkCommandBuffer& cmdBuffer_);
	void bindGraphicsPipeline(PoolId graphicsPipelineId_, VkCommandBuffer& cmdBuffer_); 
	void bindDescriptorSet(PoolId descriptorSetId_, PoolId graphicsPipelineId_, uint32_t setIndex_, VkCommandBuffer& cmdBuffer_, std::span<const uint32_t> dynamicUnitIds_ = {}); // dynamicUnitIds_ is optional, see the definition
	void drawMeshIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer& cmdBuffer_, uint32_t firstInstance_ = 0); // firstInstance_ is optional: gl_InstanceIndex runs from firstInstance_ to firstInstance_ + instanceCount_ - 1, so a draw can pick its repeat unit(s) of an SSBO descriptor.
	void endCMDsRecording(VkCommandBuffer& cmdBuffer_); 

	// Misc/Internal 
	void setViewportAndScissors(VkCommandBuffer& cmdBuffer_); 
	void presentToScreen(); 
};