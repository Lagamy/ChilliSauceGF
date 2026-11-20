#pragma once 
#include "CommandList.h"
#include "DescriptorsManager.h"
#include "Layout.h"
#include "PassesGraph.h"
#include "PipelinesManager.h"
#include "PoolId.h"
#include "PassId.h"
#include "Pass.h"
#include "MemoryEntryId.h"
#include "EntryCopyRequest.h"
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
	void addDescriptorsToShader(PoolId shaderId_, PoolId setLayoutId_, const std::vector<PoolId>& descriptorLayoutIds_); // declares that the shader reads the descriptors of those descriptor layouts in the set (derived from setLayoutId_) bound at the set index of setLayoutId_. Graphics pipelines build their pipeline layout from these, so declare before adding the pipeline. drawMeshIndexed waits for the data of exactly these. Shaders without descriptors are fine. The declaration is what lets drawMeshIndexed check these descriptors and skip the draw until their first data is copied, so every descriptor the shader reads has to be declared: nothing checks the SPIR-V, an undeclared one can be read before its data was copied
	PoolId addMesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_); 
	MemoryEntryId addAndUploadMemoryEntry(const MemoryEntryCreateParams& rParams_); // params.data and params.size describe the bytes to upload

	MemoryEntryId allocateMemoryEntry(MemoryEntryCreateParams params_, PoolId layoutId_, uint32_t repeatCount_); // the size comes from the layout, so params.size has to stay 0

	MemoryEntryId allocateMemoryEntryRaw(const MemoryEntryCreateParams& rParams_);
	void updateMemoryEntry(MemoryEntryId uploadId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);
	void copyMemoryEntry(MemoryEntryId srcEntryId_, MemoryEntryId dstEntryId_, const std::vector<MemoryCopyRegion>& regions_ = {}); // Copies ranges of one GPU_LOCAL entry (without copies) into another on the GPU. Empty regions_ = the first min(src size, dst size) bytes, a MemoryCopyRegion is {srcOffset, dstOffset, size} in entry bytes, destination ranges can't overlap, regions that touch in both entries are merged into one copy. Takes the source as it is after every write requested before it. The destination is pending until the copy completed (a draw using it skips, readbacks and updates wait behind it), the source can't be removed or overwritten before it. The destination can have writes pending, the copy goes after them
	ReadbackId requestMemoryEntryReadback(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_);
	bool isReadbackReady(ReadbackId readbackId_);
	ReadbackDataContainer mapReadback(ReadbackId readbackId_);
	void releaseReadback(ReadbackId readbackId_);

	PoolId addVerticeLayout(const char* name_, uint32_t headroom_ = 0); // headroom_: bytes of one vertex (the stride) are at least this many, so a variable added later that fits in it changes no vertex bytes. 0 = exactly what the variables take
	/*
		Runtime editing of vertex layouts, same model as the descriptor layouts: beginVertexLayoutUpdate opens an update of a layout, the add / remove functions edit it (they take the update id) and commitVertexLayoutUpdate applies it and consumes it. Ids of added variables are valid at once, survivors keep theirs.
		orderPosition_ is the place of the variable in the vertex (its bytes and its attribute location follow this order, so it has to match the shader's inputs), unique in the layout, can be sparse.
		Before any mesh or pipeline uses the layout a commit only writes it. With live ones it waits for the device and gives every uploaded mesh of the layout a new vertex entry when its bytes moved or the stride changed (GPU_LOCAL: copied on the GPU, others rewritten on the CPU), then recreates the pipelines that use it.
		The bytes of added variables are 0 after the commit (not the headroom or other unused bytes). A STATIC_ALLOCATOR mesh, or a mesh whose upload wasn't committed, that needs a new entry makes the commit throw (before anything changes). Not while recording.
	*/
	PoolId beginVertexLayoutUpdate(PoolId verticeLayoutId_);
	PoolId addVariableToVerticeLayout(PoolId updateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {});
	PoolId addVariableToVerticeLayout(PoolId updateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
	void removeVariableFromVerticeLayout(PoolId updateId_, PoolId variableId_);
	void commitVertexLayoutUpdate(PoolId updateId_);
	PoolId addDescriptorSetLayout(const char* name_, uint32_t setIndex_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_ = DescriptorSetsPerPool); // setIndex_ is the set number of the shader (layout(set = setIndex_, ...)): shader declarations and bindDescriptorSet use it, so a pipeline's shaders can't declare two different layouts with the same one. The layout adds Descriptor Pools by itself as sets are derived, each one sized for setsPerPool_ sets
	PoolId getLayoutId(const char* layoutName_); // id of the Descriptor Set Layout added with that name, throws if there is none
	/*
		Runtime editing of descriptor layouts. addDescriptorSetLayout only creates the empty set layout, what is in it is edited through updates: beginSetLayoutUpdate opens an update of the bindings of a set layout, beginDescriptorLayoutUpdate one of the members of a binding, the add / remove functions below take the update id (it knows its layout), and commitSetLayoutUpdate / commitDescriptorLayoutUpdate apply it and consume it. Nothing changes in the layout that is used until the commit (ids of added things are valid at once).
		Before anything of the layout exists (define* of a scene) a commit only writes the changes into the layouts. With live sets and memory a commit waits for the device and reallocates, copies and rewrites what the change touches: a member change gives every descriptor a new entry and carries its bytes over (a loaded GPU_LOCAL descriptor of a dynamic set by a GPU side copy, any other from the CPU copy of the set or loadFromDisk), no descriptor is changed on disk (see saveDescriptorToDisk), a binding change recreates the VkDescriptorSetLayout, the VkDescriptorSets of its sets and the pipeline layouts and pipelines that use it. Expensive, don't call it every frame. Not while recording. STATIC_ALLOCATOR sets with memory can't take a change (throws).
		Only one update of a layout is open at a time, the update of a binding added by an open set layout update has to be committed before it. A shader that declared a removed binding has to be changed by the application.
	*/
	PoolId beginSetLayoutUpdate(PoolId layoutId_);
	PoolId beginDescriptorLayoutUpdate(PoolId layoutId_, PoolId descriptorLayoutId_);
	void commitSetLayoutUpdate(PoolId updateId_);
	void commitDescriptorLayoutUpdate(PoolId updateId_);
	PoolId addBufferDescriptorLayout(PoolId setLayoutUpdateId_, const char* descriptorLayoutName_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t headroom_ = 0); // binding_ is the binding number of the shader (layout(set = N, binding = binding_)), unique in the set layout, throws otherwise. type_: UNIFORM_DESCRIPTOR or STORAGE_DESCRIPTOR, a set can mix both. headroom_: bytes one block (one dynamic unit) is at least big, 0 = what the members take. A member added later that fits in it doesn't change the size of the descriptor's entry, so a live commit keeps the entry (no new allocation, no copy). The members of the block are added with a DescriptorLayoutUpdate (beginDescriptorLayoutUpdate, addMemberToDescriptorLayout / addStructMemberToDescriptorLayout + addVariableToStructInDescriptorLayout)
	PoolId addDynamicDescriptorLayout(PoolId setLayoutUpdateId_, const char* descriptorLayoutName_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_, uint32_t headroom_ = 0); // same, but the whole block is repeated dynamicCount_ times and one of them is picked per bind (bindDescriptorSet dynamicUnitIds_) and written with setDynamicDescriptorMember...
	PoolId addImageDescriptorLayout(PoolId setLayoutUpdateId_, const char* descriptorLayoutName_, uint32_t binding_, DescriptorTypeEnum type_); // TEXTURE_DESCRIPTOR / SAMPLER_DESCRIPTOR. Not supported yet, throws
	void removeDescriptorLayout(PoolId setLayoutUpdateId_, PoolId descriptorLayoutId_); // the binding and the descriptors of its sets go at the commit
	PoolId addMemberToDescriptorLayout(PoolId descriptorLayoutUpdateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // A variable directly in the block (mat4 model;). orderPosition_ is its place in the declaration order of the shader's block (can be sparse, unique among the members, throws otherwise, the bytes are placed in this order whatever order they were added in). Returns the variable id the setters take
	PoolId addStructMemberToDescriptorLayout(PoolId descriptorLayoutUpdateId_, const char* name_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_ = 0); // A struct in the block (orderPosition_ as for addMemberToDescriptorLayout) repeated per layer, outermost first ({2, 3} = states[2][3], {1} = a single struct). Returns the struct member id, its variables are added with addVariableToStructInDescriptorLayout. headroom_: bytes the struct is at least big (0 = what its variables take), variables added later that fit in it don't change its size, so the elements of a struct array don't move
	PoolId addVariableToStructInDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // A variable of a struct member, orderPosition_ is its place inside the struct (unique there). Returns the variable id the setters take
	PoolId addVariableToStructInDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
	PoolId addMemberToDescriptorLayout(PoolId descriptorLayoutUpdateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
	void removeMemberFromDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId memberId_); // a whole member (a struct member with its variables), the other members keep their ids. 
	void removeVariableFromStructInDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId variableId_); // one variable of a struct member (a variable directly in the block goes with removeMemberFromDescriptorLayout), the variables after it move up and keep their ids
	PoolId addDescriptorSet(const DescriptorSetCreateParams& rParams_); // see DescriptorSetCreateParams: the set's descriptors are loaded and unloaded on demand
	/* The engine already loads a descriptor when a draw or pass uses it and unloads it after absenceSecondsCount seconds without use (DescriptorSetCreateParams), so these two are only for an application that wants to decide itself. descriptorId_ is the descriptor layout id. They wait for the device and are not allowed while recording. Only DYNAMIC_ALLOCATOR sets can unload; an unloaded descriptor can't be written until it was loaded again. */
	void loadDescriptor(PoolId setId_, PoolId descriptorId_);
	void unloadDescriptor(PoolId setId_, PoolId descriptorId_);
	MemoryEntryId getDescriptorMemoryEntry(PoolId setId_, PoolId descriptorId_); // the entry of a loaded descriptor (for requestMemoryEntryReadback, e.g. in saveToDisk), throws when it is unloaded. A load and a layout update that reallocates give it a new one, so ask again after them
	/* The two functions of DescriptorSetCreateParams that belong to loadFromDisk. The engine never calls them (a conversion that failed half way would already have rewritten the files): call them after the whole conversion succeeded, saveDescriptorToDisk writes the data, changeDescriptorLayoutOnDisk rewrites the file for the current layout. */
	void saveDescriptorToDisk(PoolId setId_, PoolId descriptorId_);
	void changeDescriptorLayoutOnDisk(PoolId setId_, PoolId descriptorId_);
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
	uint32_t addTaskToPass(PassId passId_, const char* name_, PoolId commandListId_); // the task is recorded from the command list (the commands are added with getCommandList(id).addX), every time it is recorded
	void setTaskCommandList(PassId passId_, uint32_t taskId_, PoolId commandListId_); // switches the list of a task made with a command list. Takes effect at the next recording. Not while recording
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
	
	// Command lists
	PoolId addCommandList(const char* name_); // names are unique, like every PoolMap. A scene that is defined again has to remove its lists first
	CommandList& getCommandList(PoolId commandListId_);
	void removeCommandList(PoolId commandListId_); // a task that still points at it throws when it is recorded

	// Commands Recording
	void beginCMDsRecording(VkCommandBuffer& cmdBuffer_); // beginCommandBuffer + beginPresentationRenderPass
	void beginCommandBuffer(VkCommandBuffer& cmdBuffer_); // also resets the recording state (bound pipeline and sets)
	void beginPresentationRenderPass(VkCommandBuffer& cmdBuffer_); // viewport, scissor and the presentation render pass on the current swapchain image
	void bindGraphicsPipeline(PoolId graphicsPipelineId_, VkCommandBuffer& cmdBuffer_); 
	void bindDescriptorSet(PoolId descriptorSetId_, PoolId graphicsPipelineId_, VkCommandBuffer& cmdBuffer_, std::span<const uint32_t> dynamicUnitIds_ = {}); // binds at the set index of the set's layout. dynamicUnitIds_ is optional, see the definition
	void drawMeshIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer& cmdBuffer_, uint32_t firstInstance_ = 0); // firstInstance_ is optional: gl_InstanceIndex runs from firstInstance_ to firstInstance_ + instanceCount_ - 1, so a draw can pick its repeat unit(s) of an SSBO descriptor.
	void endCMDsRecording(VkCommandBuffer& cmdBuffer_); // endRenderPass + endCommandBuffer
	void endRenderPass(VkCommandBuffer& cmdBuffer_);
	void endCommandBuffer(VkCommandBuffer& cmdBuffer_);

	// Misc/Internal 
	void setViewportAndScissors(VkCommandBuffer& cmdBuffer_); 
	void presentToScreen(); 
};