#include "Api.h"
#include "Globals.h"
#include "Layout.h"
#include "PoolId.h"
#include "ProjectManager.h"
#include "SubmissionBatch.h"
#include "MemoryEntry.h"
#include "MemoryEntryId.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
	// Get 
	GLFWwindow* getWindowPointer()
	{
		return Globals::appWindow; 
	}

	Instance& getInstance()
	{
		return Globals::renderer.instance; 
	}

	Device& getMainDevice()
	{
		return Globals::renderer.mainDevice; 
	}

	void switchDevice(uint32_t id_)
	{
		Globals::renderer.switchDevice(id_);
	}

	Surface& getSurface()
	{
		return Globals::renderer.surface;
	}

	Swapchain& getSwapchain()
	{
		return Globals::renderer.swapchain; 
	}
	
	GPUSceneManager& getGPUSceneManager()
	{
		return Globals::renderer.gpuSceneManager;
	}

	MemoryManager& getMemoryManager()
	{
		return Globals::renderer.memoryManager; 
	}

	PassesGraph& getPassesManager()
	{
		return  Globals::renderer.passesGraph;
	}

	PipelinesManager& getPipelinesManager()
	{
		return Globals::renderer.pipelinesManager;
	}

	DescriptorsManager& getDescriptorsManager()
	{
		return Globals::renderer.descriptorsManager;
	} 

	ReflectionSystem& getReflectionSystem()
	{
		return Globals::renderer.reflectionSystem;
	}
	

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(PoolId layoutId_)
	{
		return Globals::renderer.pipelinesManager.graphicsPipelines[layoutId_]; 
	}

	RenderPass& getPresentationRenderPass()
	{
		return Globals::renderer.presentationRenderPass; 
	}

	VkSemaphore& getSemaphore(PoolId semaphoreId_)
	{
		return Globals::renderer.syncManager.getSemaphore(semaphoreId_).get();
	}


	VkFence& getFence(PoolId fenceId_)
	{
		return Globals::renderer.syncManager.getFence(fenceId_).get();
	}


	VkQueue& getQueue(uint8_t id_)
	{
		return Globals::renderer.mainDevice.queues[id_];
	} 

	Shader& getShader(PoolId shaderId_)
	{
		return Globals::renderer.shadersManager.shaders[shaderId_]; 
	}

	const MemoryEntry& getMemoryEntry(MemoryEntryId id_)
	{
		return Globals::renderer.memoryManager.getEntry(id_);
	}

	bool isUploadInGPU(MemoryEntryId uploadId_)
	{
		return Globals::renderer.memoryManager.isUploadInGPU(uploadId_); 
	}

	


	Pass& getPass(PassId passId_)
	{
		return Globals::renderer.passesGraph.getPass(passId_);
	}

	PoolId getCurrentSwapchainImageUseFinishedSemaphore()
	{
		return getCurrentSwapchainImage().imageUseFinishedSemaphoreId;
	} 

	PoolId getCurrentFrameImageIsAcquiredSemaphore()
	{
		return getCurrentFrameResources().imageAcquiredSemaphoreId;
	} 

	uint64_t getIndividualUpdateMemBlockSize()
	{
		return Globals::individualUpdateMemBlockSize;
	} 

	Buffer& getGPUBuffer(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_)
	{
		//if(allocatorType_ == STATIC_ALLOCATOR)
		//{
			return Globals::renderer.memoryManager.staticAllocator.getBuffer(uploadType_, GPU_LOCAL);
		//}
	}
	
	uint32_t getGPUBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_)
	{
		return  Globals::renderer.memoryManager.staticAllocator.gpuLocalHeap.bufferFirstByte[uploadType_]; 
	}

	uint32_t& getCurrentImageIndex()
	{
		return Globals::renderer.imageIndex; 
	}


	uint32_t& getCurrentFrameIndex()
	{
		return Globals::renderer.currentFrame;
	} 

	FrameResources& getCurrentFrameResources()
	{
		return Globals::renderer.framesResources[Globals::renderer.currentFrame]; 
	}

	Image& getCurrentSwapchainImage()
	{
		return getSwapchain().renderTargets[getCurrentImageIndex()];
	}

	VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CmdLifetimeEnum poolType_, uint32_t id_)
	{
			
		if(poolType_ == FRAME)
		{
			return Globals::renderer.framesResources[Globals::renderer.currentFrame].frameCmdPools.getPoolByQueue(queueFamily_).commandBuffers.buffers[id_];
		}
		else
		{
			return Globals::renderer.oneShotCommandPools.getPoolByQueue(queueFamily_).commandBuffers.buffers[id_];
		}
	}

	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CmdLifetimeEnum poolType_)
	{
		if(poolType_ == FRAME)
		{
			return Globals::renderer.framesResources[Globals::renderer.currentFrame].frameCmdPools.getPoolByQueue(queueFamily_).get();
		}
		else
		{
			return Globals::renderer.oneShotCommandPools.getPoolByQueue(queueFamily_).get();
		}
	}


	ReflectionLayout& getVerticeLayout(PoolId layoutId_)
	{
		return Globals::renderer.reflectionSystem.layoutsPerBufferType[VERTEX_BUFFER - 1][layoutId_]; // - 1 due to INDEX layout not existing  
	}

	DescriptorSet& getDescriptorSet(PoolId setId_)
	{
		return Globals::renderer.descriptorsManager.descriptorSet[setId_];
	}

	DescriptorSetLayout& getDescriptorSetLayout(PoolId layoutId_)
	{
		return Globals::renderer.descriptorsManager.layouts[layoutId_];
	}

	Mesh& getMesh(PoolId meshId_)
	{
		return Globals::renderer.resourcesManager.meshes[meshId_];
	} 

	
	void setEnvironmentSetupFunction(ProjectFunc setupEnvironmetFunc_)
	{
		Globals::renderer.projectManager.setupEnvironment = setupEnvironmetFunc_;
	}

	void setFramesAtFlightCount(uint32_t count_)
	{
		Globals::renderer.framesAtFlightCount = count_;
	}

	void setIndividualUpdateMemBlockSize(uint64_t value_)
	{
		Globals::individualUpdateMemBlockSize = value_; 
	}


	void resetFences(std::span<VkFence> fences_)
	{
		vkResetFences(getMainDevice().logicalDevice, fences_.size(), fences_.data());	
	}
	
	void resetFence(PoolId fenceId_)
	{
		vkResetFences(getMainDevice().logicalDevice, 1, &getFence(fenceId_));	
	}
	
	bool wasFenceSignaled(PoolId fenceId_)
	{
		VkResult result = vkGetFenceStatus(getMainDevice().logicalDevice, getFence(fenceId_));
		return result == VK_SUCCESS; 
	}

	// Add
	
	void addDeviceExtension(const char* name_)
	{
		Globals::requiredDeviceExtensions.emplace_back(name_); 
	} 

	
	PoolId addSemaphore()
	{
		return Globals::renderer.syncManager.addSemaphore();
	}
		
	PoolId addFence(bool createSignaled_)
	{
		return Globals::renderer.syncManager.addFence(createSignaled_);
	}

	PoolId addShader(const char* name_, const char* path_)
	{
		return Globals::renderer.shadersManager.shaders.add(name_, Disk::executablePath + path_);  
	}

	void addDescriptorsToShader(PoolId shaderId_, PoolId setLayoutId_, const std::vector<PoolId>& descriptorLayoutIds_)
	{
		uint32_t setIndex = getDescriptorsManager().layouts[setLayoutId_].setIndex;
		for(PoolId descriptorLayoutId : descriptorLayoutIds_)
		{
			getShader(shaderId_).addDescriptor(setIndex, setLayoutId_, descriptorLayoutId);
		}
	}

	void removeShader(PoolId& id_)
	{
		Globals::renderer.shadersManager.shaders.remove(id_); 
	}



	MemoryEntryId addAndUploadMemoryEntry(const MemoryEntryCreateParams& rParams_)
	{
		return Globals::renderer.memoryManager.addEntry(rParams_);
	}

	/* The layout is looked up in the bufferType_ list. The INDEX layout doesn't exist, and the TEXTURE and SAMPLER lists aren't there yet. */
	MemoryEntryId allocateMemoryEntry(MemoryEntryCreateParams params_, PoolId layoutId_, uint32_t repeatCount_)
	{
		if(params_.size != 0)
		{
			throw std::runtime_error(std::format("Api: Entry {} sets a size, allocateMemoryEntry takes it from the layout. Use allocateMemoryEntryRaw.", params_.name == nullptr ? "" : params_.name));
		}
		#ifdef ENGINE_DEBUG
			if(params_.bufferType < VERTEX_BUFFER || params_.bufferType > STORAGE_BUFFER)
			{
				throw std::runtime_error(std::format("Api: Can't allocate {} from a layout, buffer type {} has no layouts. Use allocateMemoryEntryRaw.", params_.name == nullptr ? "" : params_.name, static_cast<uint32_t>(params_.bufferType)));
			}
		#endif

		params_.size = Globals::renderer.reflectionSystem.layoutsPerBufferType[params_.bufferType - 1][layoutId_].size * repeatCount_; // - 1 due to INDEX layout not existing
		return Globals::renderer.memoryManager.allocateEntry(params_);
	}

	MemoryEntryId allocateMemoryEntryRaw(const MemoryEntryCreateParams& rParams_)
	{
		return Globals::renderer.memoryManager.allocateEntry(rParams_);
	}

	void updateMemoryEntry(MemoryEntryId uploadId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
	{
		Globals::renderer.memoryManager.updateEntry(uploadId_, data_, inSrcOffset_, inEntryOffset_, size_);
	}

	void copyMemoryEntry(MemoryEntryId srcEntryId_, MemoryEntryId dstEntryId_, const std::vector<MemoryCopyRegion>& regions_)
	{
		Globals::renderer.memoryManager.copyEntry(srcEntryId_, dstEntryId_, regions_);
	}

	ReadbackId requestMemoryEntryReadback(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_)
	{
		return Globals::renderer.memoryManager.requestReadback(entryId_, inEntryOffset_, size_);
	}

	bool isReadbackReady(ReadbackId readbackId_)
	{
		return Globals::renderer.memoryManager.isReadbackReady(readbackId_);
	}

	ReadbackDataContainer mapReadback(ReadbackId readbackId_)
	{
		return Globals::renderer.memoryManager.getReadbackContainer(readbackId_);
	}

	void releaseReadback(ReadbackId readbackId_)
	{
		Globals::renderer.memoryManager.releaseReadback(readbackId_);
	}

	PoolId addVerticeLayout(const char* name_, uint32_t headroom_)
	{
		return Globals::renderer.reflectionSystem.layoutsPerBufferType[VERTEX_BUFFER - 1].add(name_, NONE, headroom_); 
	}

	PoolId beginVertexLayoutUpdate(PoolId verticeLayoutId_)
	{
		return Globals::renderer.reflectionSystem.beginVertexLayoutUpdate(verticeLayoutId_);
	}

	PoolId addVariableToVerticeLayout(const char* name_, PoolId updateId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.reflectionSystem.addVariableToVerticeLayout(name_, updateId_, orderPosition_, dataType_, repeatCountPerDimension_);
	}

	PoolId addVariableToVerticeLayout(const char* name_, PoolId updateId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.reflectionSystem.addVariableToVerticeLayout(name_, updateId_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
	}

	void removeVariableFromVerticeLayout(PoolId updateId_, PoolId variableId_)
	{
		Globals::renderer.reflectionSystem.removeVariableFromVerticeLayout(updateId_, variableId_);
	}

	void commitVertexLayoutUpdate(PoolId updateId_)
	{
		Globals::renderer.reflectionSystem.commitVertexLayoutUpdate(updateId_);
	}

	PoolId addMesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_)
	{
		return Globals::renderer.resourcesManager.meshes.add(name_, name_, verticeLayoutId_, repeatCount_); 
	} 

	PoolId addImage(const char* name_, bool perFrameInFlight_, VkImageUsageFlags usageFlags_, VkFormat format_, VkExtent3D extent_, VkImageType imageType_, uint32_t mipLevelCount_, uint32_t arrayLayerCount_, VkImageCreateFlags flags_, bool cpuBitmapEdits_)
	{
		PoolId imagePackId = Globals::renderer.resourcesManager.imagePacks.add(name_, perFrameInFlight_);
		Globals::renderer.resourcesManager.imagePacks[imagePackId].init(name_, usageFlags_, format_, extent_, imageType_, mipLevelCount_, arrayLayerCount_, flags_, cpuBitmapEdits_);
		return imagePackId;
	}

	Image& getImage(PoolId imagePackId_)
	{
		return Globals::renderer.resourcesManager.imagePacks[imagePackId_].get();
	}

	PoolId addDescriptorSetLayout(const char* name_, uint32_t setIndex_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_)
	{
		return Globals::renderer.descriptorsManager.addDescriptorSetLayout(name_, setIndex_, shaderStages_, setsPerPool_);
	}

	PoolId getLayoutId(const char* layoutName_)
	{
		return Globals::renderer.descriptorsManager.getLayoutId(layoutName_);
	}

	PoolId beginSetLayoutUpdate(PoolId layoutId_)
	{
		return Globals::renderer.descriptorsManager.beginSetLayoutUpdate(layoutId_);
	}

	PoolId beginDescriptorLayoutUpdate(PoolId layoutId_, PoolId descriptorLayoutId_)
	{
		return Globals::renderer.descriptorsManager.beginDescriptorLayoutUpdate(layoutId_, descriptorLayoutId_);
	}

	void commitSetLayoutUpdate(PoolId updateId_)
	{
		Globals::renderer.descriptorsManager.commitSetLayoutUpdate(updateId_);
	}

	void commitDescriptorLayoutUpdate(PoolId updateId_)
	{
		Globals::renderer.descriptorsManager.commitDescriptorLayoutUpdate(updateId_);
	}

	PoolId addBufferDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t headroom_)
	{
		return Globals::renderer.descriptorsManager.addBufferDescriptorLayout(descriptorLayoutName_, setLayoutUpdateId_, binding_, type_, headroom_);
	}

	PoolId addDynamicDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_, uint32_t headroom_)
	{
		return Globals::renderer.descriptorsManager.addDynamicDescriptorLayout(descriptorLayoutName_, setLayoutUpdateId_, binding_, type_, dynamicCount_, headroom_);
	}

	PoolId addImageDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_)
	{
		return Globals::renderer.descriptorsManager.addImageDescriptorLayout(descriptorLayoutName_, setLayoutUpdateId_, binding_, type_);
	}

	void removeDescriptorLayout(PoolId setLayoutUpdateId_, PoolId descriptorLayoutId_)
	{
		Globals::renderer.descriptorsManager.removeDescriptorLayout(setLayoutUpdateId_, descriptorLayoutId_);
	}

	PoolId addMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.descriptorsManager.addMemberToDescriptorLayout(name_, descriptorLayoutUpdateId_, orderPosition_, dataType_, repeatCountPerDimension_);
	}

	PoolId addMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.descriptorsManager.addMemberToDescriptorLayout(name_, descriptorLayoutUpdateId_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
	}

	PoolId addStructMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_)
	{
		return Globals::renderer.descriptorsManager.addStructMemberToDescriptorLayout(name_, descriptorLayoutUpdateId_, orderPosition_, repeatCountPerDimension_, headroom_);
	}

	PoolId addVariableToStructInDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.descriptorsManager.addVariableToStructInDescriptorLayout(name_, descriptorLayoutUpdateId_, structMemberId_, orderPosition_, dataType_, repeatCountPerDimension_);
	}

	PoolId addVariableToStructInDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.descriptorsManager.addVariableToStructInDescriptorLayout(name_, descriptorLayoutUpdateId_, structMemberId_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
	}

	void removeMemberFromDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId memberId_)
	{
		Globals::renderer.descriptorsManager.removeMemberFromDescriptorLayout(descriptorLayoutUpdateId_, memberId_);
	}

	void removeVariableFromStructInDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId variableId_)
	{
		Globals::renderer.descriptorsManager.removeVariableFromStructInDescriptorLayout(descriptorLayoutUpdateId_, variableId_);
	}

	PoolId addDescriptorSet(const DescriptorSetCreateParams& rParams_)
	{
		return Globals::renderer.descriptorsManager.addDescriptorSet(rParams_);
	}

	void loadDescriptor(PoolId setId_, PoolId descriptorId_)
	{
		Globals::renderer.descriptorsManager.loadDescriptor(setId_, descriptorId_);
	}

	MemoryEntryId getDescriptorMemoryEntry(PoolId setId_, PoolId descriptorId_)
	{
		return Globals::renderer.descriptorsManager.getDescriptorMemoryEntry(setId_, descriptorId_);
	}

	void saveDescriptorToDisk(PoolId setId_, PoolId descriptorId_)
	{
		Globals::renderer.descriptorsManager.saveDescriptorToDisk(setId_, descriptorId_);
	}

	void changeDescriptorLayoutOnDisk(PoolId setId_, PoolId descriptorId_)
	{
		Globals::renderer.descriptorsManager.changeDescriptorLayoutOnDisk(setId_, descriptorId_);
	}

	void unloadDescriptor(PoolId setId_, PoolId descriptorId_)
	{
		Globals::renderer.descriptorsManager.unloadDescriptor(setId_, descriptorId_);
	}

	void loadMesh(PoolId meshId_)
	{
		Globals::renderer.resourcesManager.loadMesh(meshId_);
	}

	void unloadMesh(PoolId meshId_)
	{
		Globals::renderer.resourcesManager.unloadMesh(meshId_);
	}

	MemoryEntryId getMeshVertexEntry(PoolId meshId_)
	{
		return Globals::renderer.resourcesManager.getMeshVertexEntry(meshId_);
	}

	void saveMeshToDisk(PoolId meshId_)
	{
		Globals::renderer.resourcesManager.saveMeshToDisk(meshId_);
	}

	void changeMeshLayoutOnDisk(PoolId meshId_)
	{
		Globals::renderer.resourcesManager.changeMeshLayoutOnDisk(meshId_);
	}

	void removeDescriptorSet(PoolId setId_)
	{
		Globals::renderer.descriptorsManager.removeDescriptorSet(setId_);
	}

	void removeDescriptorSetLayout(PoolId layoutId_)
	{
		Globals::renderer.descriptorsManager.removeDescriptorSetLayout(layoutId_);
	}

	PoolId addGraphicsPipelineLayout(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_)
	{
		return Globals::renderer.pipelinesManager.addGraphicsPipeline(name_, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_, rRenderpass_, subpassId_);
	} 

	PoolId addPushConstantLayout(const char* name_)
	{
		return Globals::renderer.pipelinesManager.pushConstantLayouts.add(name_, name_);
	}

	PoolId getGraphicsPipelineId(const char* name_)
	{
		return Globals::renderer.pipelinesManager.graphicsPipelines.getIdByName(name_);
	}

	PushConstantLayout& getPushConstantLayout(PoolId pushConstantLayoutId_)
	{
		return Globals::renderer.pipelinesManager.pushConstantLayouts[pushConstantLayoutId_];
	}

	PoolId beginPushConstantLayoutUpdate(PoolId pushConstantLayoutId_)
	{
		return Globals::renderer.pipelinesManager.beginPushConstantLayoutUpdate(pushConstantLayoutId_);
	}

	void commitPushConstantLayoutUpdate(PoolId updateId_)
	{
		Globals::renderer.pipelinesManager.commitPushConstantLayoutUpdate(updateId_);
	}

	PoolId addMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.pipelinesManager.addMemberToPushConstantLayout(name_, pushConstantLayoutUpdateId_, orderPosition_, dataType_, repeatCountPerDimension_);
	}

	PoolId addMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.pipelinesManager.addMemberToPushConstantLayout(name_, pushConstantLayoutUpdateId_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
	}

	PoolId addStructMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_)
	{
		return Globals::renderer.pipelinesManager.addStructMemberToPushConstantLayout(name_, pushConstantLayoutUpdateId_, orderPosition_, repeatCountPerDimension_, headroom_);
	}

	PoolId addVariableToStructInPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.pipelinesManager.addVariableToStructInPushConstantLayout(name_, pushConstantLayoutUpdateId_, structMemberId_, orderPosition_, dataType_, repeatCountPerDimension_);
	}

	PoolId addVariableToStructInPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
	{
		return Globals::renderer.pipelinesManager.addVariableToStructInPushConstantLayout(name_, pushConstantLayoutUpdateId_, structMemberId_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
	}

	void removeMemberFromPushConstantLayout(PoolId pushConstantLayoutUpdateId_, PoolId memberId_)
	{
		Globals::renderer.pipelinesManager.removeMemberFromPushConstantLayout(pushConstantLayoutUpdateId_, memberId_);
	}

	void removeVariableFromStructInPushConstantLayout(PoolId pushConstantLayoutUpdateId_, PoolId variableId_)
	{
		Globals::renderer.pipelinesManager.removeVariableFromStructInPushConstantLayout(pushConstantLayoutUpdateId_, variableId_);
	}

	void addPushConstantLayoutToGraphicsPipeline(PoolId graphicsPipelineId_, PoolId pushConstantLayoutId_, VkShaderStageFlags stages_)
	{
		Globals::renderer.pipelinesManager.addPushConstantLayoutToPipeline(graphicsPipelineId_, pushConstantLayoutId_, stages_);
	}

	PoolId getPushConstantLayoutId(const char* name_, PoolId graphicsPipelineId_)
	{
		return Globals::renderer.pipelinesManager.getPushConstantLayoutId(name_, graphicsPipelineId_);
	}

	uint32_t getPushConstantOffset(PoolId graphicsPipelineId_, PoolId pushConstantLayoutId_)
	{
		return Globals::renderer.pipelinesManager.getPushConstantOffset(graphicsPipelineId_, pushConstantLayoutId_);
	}

	void addPageForDynamicMemoryEntries(MemoryVisabilityEnum memoryVisability_, uint32_t upperBoundForEntrySize_, StorageUnitEnum upperBoundUnit_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_)
	{
		getMemoryManager().dynamicAllocator.addPage(memoryVisability_, upperBoundForEntrySize_, upperBoundUnit_, memoryBlockSize_, memoryBlockSizeUnit_);
	}

	void addPageForStagingUploads(uint32_t upperBoundForRangeSize_, StorageUnitEnum upperBoundUnit_, uint32_t heapSize_, StorageUnitEnum heapSizeUnit_, uint32_t maxDormantHeaps_)
	{
		getMemoryManager().stagingAllocator.addPage(UPLOAD, upperBoundForRangeSize_, upperBoundUnit_, heapSize_, heapSizeUnit_, maxDormantHeaps_);
	}

	void addPageForStagingReadbacks(uint32_t upperBoundForRangeSize_, StorageUnitEnum upperBoundUnit_, uint32_t heapSize_, StorageUnitEnum heapSizeUnit_, uint32_t maxDormantHeaps_)
	{
		getMemoryManager().stagingAllocator.addPage(READBACK, upperBoundForRangeSize_, upperBoundUnit_, heapSize_, heapSizeUnit_, maxDormantHeaps_);
	}

	void createAllPipelines()
	{
		Globals::renderer.pipelinesManager.createAllPipelines(); 
	}

	void destroyAllPipelines()
	{
		Globals::renderer.pipelinesManager.destroyAllPipelines();
	}
	
	PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_)
    {
		return Globals::renderer.passesGraph.addPass(name_, cmdType_, queueFamily_, signalFenceId_);
	}

	PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_)
	{
		return Globals::renderer.passesGraph.addPass(name_, cmdType_, queueFamily_);
	} 

    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc signalFenceRetrivalFunc_)
	{
		return Globals::renderer.passesGraph.addPass(name_, cmdType_, queueFamily_, signalFenceRetrivalFunc_);
	} 

	uint32_t addTaskToPass(const char* name_, PassId passId_, PoolId commandListId_)
	{
		return Globals::renderer.passesGraph.addTaskToPass(name_, passId_, commandListId_);
	}

	void setTaskCommandList(PassId passId_, uint32_t taskId_, PoolId commandListId_)
	{
		Globals::renderer.passesGraph.setTaskCommandList(passId_, taskId_, commandListId_);
	}

	PoolId addCommandList(const char* name_)
	{
		return Globals::renderer.commandLists.add(name_);
	}

	CommandList& getCommandList(PoolId commandListId_)
	{
		return Globals::renderer.commandLists[commandListId_];
	}

	void removeCommandList(PoolId commandListId_)
	{
		Globals::renderer.commandLists.remove(commandListId_);
	}

	void addWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_)
	{
		Globals::renderer.passesGraph.getPass(passId_).tasks[taskId_].addWaitSemaphore(waitSemaphoreId_, pipelineStage_);
	} 

	void addSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, PoolId signalSemaphoreId_)
	{
		Globals::renderer.passesGraph.getPass(passId_).tasks[taskId_].addSignalSemaphore(signalSemaphoreId_);
	}

	void addDynamicWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_)
	{
		Globals::renderer.passesGraph.addDynamicWaitSemaphoreToTask(passId_, taskId_, semaphoreRetrivalFunc_, pipelineStage_);
	} 

    void addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_)
	{
		Globals::renderer.passesGraph.addDynamicSignalSemaphoreToTask(passId_, taskId_, semaphoreRetrivalFunc_);
	}
	
	void enablePass(PassId passId_)
	{
		Globals::renderer.passesGraph.enablePass(passId_); 
	} 

    void disableFramePass(PassId passId_) // Since oneshot - self disables
	{
		Globals::renderer.passesGraph.disableFramePass(passId_);
	}

	void removeFence(PoolId id_)
	{
		Globals::renderer.syncManager.fences.remove(id_);
	}

	void removeSemaphore(PoolId id_)
	{
		Globals::renderer.syncManager.semaphores.remove(id_);
	}

	void removeUpload(MemoryEntryId uploadId_)
	{
		Globals::renderer.memoryManager.removeDynamicEntry(uploadId_);
	}

	void beginCMDsRecording(VkCommandBuffer &cmdBuffer_)
	{
		beginCommandBuffer(cmdBuffer_);
		beginPresentationRenderPass(cmdBuffer_);
	}

	void beginCommandBuffer(VkCommandBuffer &cmdBuffer_)
	{
		VkCommandBufferBeginInfo beginInfo = {}; 
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
		vkBeginCommandBuffer(cmdBuffer_, &beginInfo);
		Globals::renderer.recordingPipelineId = UninitializedPoolId;
		Globals::renderer.recordingSetIds.clear();
	}

	void beginPresentationRenderPass(VkCommandBuffer &cmdBuffer_)
	{
		setViewportAndScissors(cmdBuffer_);
		VkClearValue clearColor = {};
		clearColor.color = {0.0f, 0.0f, 0.0f, 1.0f};

		VkRenderPassBeginInfo renderPassInfo = {}; 
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; 
		renderPassInfo.renderPass = getPresentationRenderPass().get(); 
		renderPassInfo.framebuffer = getSwapchain().framebuffers[getCurrentImageIndex()].get(); // We use ImageIndex here and not currentFrameInFlight due to images not really being 1:1 with frames(i can be rendered faster than other for some reason, and so 2 image will be at the third frame) 
		renderPassInfo.renderArea = { 
			.offset = {0, 0}, 
			.extent = getSwapchain().extent
		};
		renderPassInfo.clearValueCount = 1; 
		renderPassInfo.pClearValues = &clearColor; 
	
		vkCmdBeginRenderPass(cmdBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // VK_SUBPASS_CONTENTS_INLINE - mean The commands for this subpass will be recorded directly into this primary command buffer.
	}

	void bindGraphicsPipeline(PoolId graphicsPipelineId_, VkCommandBuffer &cmdBuffer_) 
	{
		Globals::renderer.recordingPipelineId = graphicsPipelineId_;
		vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, getGraphicsPipeline(graphicsPipelineId_).get());
	}

	/*
		The set is bound at the set index of its layout (DescriptorSetLayout::setIndex). It has to be compatible with the set layout at that index of the pipeline's layout (descriptor set layout compatibility), that's the only thing Vulkan requires for the bind. It is always bound, even while some of its descriptors have no data yet: every binding was written by DescriptorSet::create, and a shader only reads the bindings it uses.
		dynamicUnitIds_ (optional): the repeat unit each dynamic descriptor layout sees in this bind (one id per dynamic descriptor, in binding order, empty = unit 0). The framework turns them into the padded byte offsets, so call sites never touch strides.
		The set is remembered for the rest of the recording (Renderer::recordingSetIds), drawMeshIndexed looks up the descriptors its shaders declared (Shader::addDescriptor) in it.
	*/
	void bindDescriptorSet(PoolId descriptorSetId_, PoolId graphicsPipelineId_, VkCommandBuffer &cmdBuffer_, std::span<const uint32_t> dynamicUnitIds_)
	{
		DescriptorSet& rSet = getDescriptorSet(descriptorSetId_);
		uint32_t setIndex = getDescriptorsManager().layouts[rSet.setLayout].setIndex;
		VkPipelineLayout pipelineLayout = getPipelinesManager().pipelineLayouts[getGraphicsPipeline(graphicsPipelineId_).pipelineLayoutId].get();
		if(Globals::renderer.recordingSetIds.size() <= setIndex)
		{
			Globals::renderer.recordingSetIds.resize(setIndex + 1, UninitializedPoolId);
		}
		Globals::renderer.recordingSetIds[setIndex] = descriptorSetId_;
		std::vector<uint32_t> dynamicOffsets = rSet.getDynamicOffsets(dynamicUnitIds_);
		VkDescriptorSet descriptorSet = rSet.get();
		vkCmdBindDescriptorSets(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setIndex, 1, &descriptorSet, static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
	}

	/*
		Pushes with the layout of the pipeline, with the shader stages of the use (VUID-vkCmdPushConstants-offset-01795 / 01796: every byte of the pushed range has to be in a range of the layout that has every stage in stageFlags, and stageFlags has to contain all stages of the ranges that overlap it, so the use's own stages are passed whole).
		A command added with a pointer makes the bytes now, so they hold the value of the variable at this recording; one added with a value pushes the bytes it took when it was added. A column is pushed as one run per element. Pushed values stay valid for pipelines with a compatible layout; binding a pipeline whose push constant ranges differ invalidates them (pipeline layout compatibility), which is why the command goes after the bind of its pipeline.
	*/
	void setPushConstantMemberValue(const SetPushConstantMemberValueParams& rParams_, PoolId graphicsPipelineId_, VkCommandBuffer& cmdBuffer_)
	{
		PipelineLayout& rPipelineLayout = getPipelinesManager().pipelineLayouts[getGraphicsPipeline(graphicsPipelineId_).pipelineLayoutId];
		const PushConstantUse& rUse = rPipelineLayout.getPushConstantUse(rParams_.pushConstantLayoutId);
		std::vector<PushConstantBytes> runs;
		if(!rParams_.cached)
		{
			runs = getPipelinesManager().pushConstantLayouts[rParams_.pushConstantLayoutId].makeBytes(rParams_.write);
		}
		const std::vector<PushConstantBytes>& rValue = rParams_.cached ? rParams_.cachedBytes : runs;
		for(const PushConstantBytes& rRun : rValue)
		{
			vkCmdPushConstants(cmdBuffer_, rPipelineLayout.get(), rUse.stages, rUse.offset + static_cast<uint32_t>(rRun.inBlockOffset), static_cast<uint32_t>(rRun.bytes.size()), rRun.bytes.data());
		}
	}

	/* The set bound at setIndex in the command buffer being recorded. Throws when a shader declares a descriptor of a set index nothing was bound to, since the draw would read an unbound set. */
	static DescriptorSet& getRecordingSet(uint32_t setIndex_)
	{
		if(Globals::renderer.recordingSetIds.size() <= setIndex_ || Globals::renderer.recordingSetIds[setIndex_] == UninitializedPoolId)
		{
			throw std::runtime_error(std::format("Draw: A shader uses a descriptor of set index {}, but no set is bound there.", setIndex_));
		}
		return getDescriptorSet(Globals::renderer.recordingSetIds[setIndex_]);
	}

	/*
		A draw may only read the descriptors its shader declared once they are loaded and their first data was copied (DescriptorSet::isDescriptorSettled), later updates are ordered by the upload batch barriers. Descriptors the shader doesn't use don't matter, even when they share a set with one that is pending.
		Every declared descriptor is marked used, also after one failed: that restarts its absence time and requests a load of an unloaded one (DescriptorsManager::updateLoading serves it at the next frame start).
		A draw that fails this is skipped for the frame and recorded again on a later one (the command buffer is re-recorded every frame), like drawMeshIndexed skips until its uploads are in the GPU.
	*/
	static bool areShaderDescriptorsSettled(const Shader& rShader_)
	{
		bool settled = true;
		for(const ShaderDescriptorUse& rUse : rShader_.descriptors)
		{
			DescriptorSet& rSet = getRecordingSet(rUse.setIndex);
			rSet.markUsed(rUse.descriptorId);
			if(!rSet.isDescriptorSettled(rUse.descriptorId))
			{
				settled = false;
			}
		}
		return settled;
	}

	/* Queued only for draws that are recorded, so a descriptor that was skipped keeps accepting its first writes. The entries are locked after the frame's submit (DescriptorsManager::lockBoundDescriptors). */
	static void queueShaderDescriptorLocks(const Shader& rShader_)
	{
		for(const ShaderDescriptorUse& rUse : rShader_.descriptors)
		{
			getDescriptorsManager().queueLockOnSubmit(Globals::renderer.recordingSetIds[rUse.setIndex], rUse.descriptorId);
		}
	}

	void drawMeshIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer &cmdBuffer_, uint32_t firstInstance_)
	{
		Mesh& rMesh = getMesh(meshId_);
		rMesh.markUsed();
		GraphicsPipeline& rPipeline = getGraphicsPipeline(Globals::renderer.recordingPipelineId);
		Shader& rVertexShader = getShader(rPipeline.vertexShaderId);
		Shader& rFragmentShader = getShader(rPipeline.fragmentShaderId);
		/* Both are checked before deciding, so the descriptors of the second shader are marked used even when the first one fails. */
		bool vertexSettled = areShaderDescriptorsSettled(rVertexShader);
		bool fragmentSettled = areShaderDescriptorsSettled(rFragmentShader);
		if(!vertexSettled || !fragmentSettled)
		{
			return;
		}
		MemoryManager& rMemoryManager = Globals::renderer.memoryManager;
		/* An unloaded mesh is skipped like one whose uploads are still in flight (Mesh::isSettled), markUsed above requested its load. */
		if(rMesh.isSettled())
		{
			queueShaderDescriptorLocks(rVertexShader);
			queueShaderDescriptorLocks(rFragmentShader);

			const MemoryEntry& indexUpload = getMemoryEntry(rMesh.ibMemoryUploadId); 
			const MemoryEntry& vertexUpload = getMemoryEntry(rMesh.vbMemoryUploadId);

			/* A mesh with copies (READ_AND_WRITE with a copyCountMode) binds the copy of this frame: it is the one syncCopiesToFrame refreshed after the frame's fence wait. */
			const MemoryEntry& vertexCopy = rMemoryManager.getFrameCopy(vertexUpload);
			const MemoryEntry& indexCopy = rMemoryManager.getFrameCopy(indexUpload);

			VkBuffer vertexBuffers[] = { rMemoryManager.getBuffer(vertexCopy).get() };
			VkDeviceSize vOffsets[] = { vertexCopy.inBufferFirstByte };
			vkCmdBindVertexBuffers(cmdBuffer_, 0, 1, vertexBuffers, vOffsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(cmdBuffer_, rMemoryManager.getBuffer(indexCopy).get(), indexCopy.inBufferFirstByte, VK_INDEX_TYPE_UINT32);
		
			// Draw indexed 
			vkCmdDrawIndexed(cmdBuffer_, getMesh(meshId_).indices.size(), instanceCount_, 0, 0, firstInstance_); 
		}
	}

	void endCMDsRecording(VkCommandBuffer &cmdBuffer_)
	{
		endRenderPass(cmdBuffer_);
		endCommandBuffer(cmdBuffer_);
	}

	void endRenderPass(VkCommandBuffer &cmdBuffer_)
	{
		vkCmdEndRenderPass(cmdBuffer_);
	}

	void endCommandBuffer(VkCommandBuffer &cmdBuffer_)
	{
		vkEndCommandBuffer(cmdBuffer_);
	}


	void setViewportAndScissors(VkCommandBuffer& cmdBuffer_)
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width  = static_cast<float>(getSwapchain().extent.width);
		viewport.height = static_cast<float>(getSwapchain().extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(cmdBuffer_, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = getSwapchain().extent;

		vkCmdSetScissor(cmdBuffer_, 0, 1, &scissor);
	}

	void presentToScreen()
	{
		VkSemaphore* pImageUseFinishedSemaphore = &getSemaphore(getCurrentSwapchainImage().imageUseFinishedSemaphoreId);
		// Present Frame 
		VkPresentInfoKHR presentInfo = {}; 
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; 
		presentInfo.waitSemaphoreCount = 1; 
		presentInfo.pWaitSemaphores = pImageUseFinishedSemaphore; 
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &getSwapchain().vkHandle; 
		presentInfo.pImageIndices = &getCurrentImageIndex();

		VkResult result = vkQueuePresentKHR(getMainDevice().queues[PresentationQueueId], &presentInfo);
		if(result != VK_SUCCESS)
		{
			throw std::runtime_error("Presentation: Failed to present Image");
		} 
	}
}
