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
		//if(allocatorType_ == STATIC)
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

	void addDescriptorToShader(PoolId shaderId_, uint32_t setIndex_, PoolId blueprintId_)
	{
		getShader(shaderId_).addDescriptor(setIndex_, blueprintId_);
	}

	void removeShader(PoolId& id_)
	{
		Globals::renderer.shadersManager.shaders.remove(id_); 
	}



	MemoryEntryId addAndUploadMemoryEntry(const char* name_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_, const void* data_, VkDeviceSize size_, bool readOnly_)
	{
		return Globals::renderer.memoryManager.addEntry(name_, data_, size_, allocatorType_, memoryVisability_, uploadType_, readOnly_);
	}

	/* The layout is looked up in the bufferType_ list. The INDEX layout doesn't exist, and the TEXTURE and SAMPLER lists aren't there yet. */
	MemoryEntryId allocateMemoryEntry(const char* name_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum bufferType_, PoolId layoutId_, uint32_t repeatCount_, bool readOnly_)
	{
		#ifdef ENGINE_DEBUG
			if(bufferType_ < VERTEX_BUFFER || bufferType_ > STORAGE_BUFFER)
			{
				throw std::runtime_error(std::format("Api: Can't allocate {} from a layout, buffer type {} has no layouts. Use allocateMemoryEntryRaw.", name_, static_cast<uint32_t>(bufferType_)));
			}
		#endif

		VkDeviceSize size = Globals::renderer.reflectionSystem.layoutsPerBufferType[bufferType_ - 1][layoutId_].size * repeatCount_; // - 1 due to INDEX layout not existing
		return Globals::renderer.memoryManager.allocateEntry(name_, allocatorType_, memoryVisability_, bufferType_, size, readOnly_);
	}

	MemoryEntryId allocateMemoryEntryRaw(const char* name_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum bufferType_, VkDeviceSize size_, bool readOnly_)
	{
		return Globals::renderer.memoryManager.allocateEntry(name_, allocatorType_, memoryVisability_, bufferType_, size_, readOnly_);
	}

	void updateMemoryEntry(MemoryEntryId uploadId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
	{
		Globals::renderer.memoryManager.updateEntry(uploadId_, data_, inSrcOffset_, inEntryOffset_, size_);
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

	PoolId addVerticeLayout(const char* name_)
	{
		return Globals::renderer.reflectionSystem.layoutsPerBufferType[VERTEX_BUFFER - 1].add(name_); 
	}

	PoolId addMemberToVerticeLayout(PoolId layoutId_, const char* name_, DataTypeEnum dataType_)
	{
		return getVerticeLayout(layoutId_).addMemberBlueprint(name_, dataType_);
	}

	PoolId addMesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_)
	{
		return Globals::renderer.resourcesManager.meshes.add(name_, name_, verticeLayoutId_, repeatCount_); 
	} 

	PoolId addDescriptorSetLayout(const char* name_, DescriptorTypeEnum descriptorType_, VkShaderStageFlags shaderStages_)
	{
		return Globals::renderer.descriptorsManager.addDescriptorSetLayout(name_, descriptorType_, shaderStages_);
	}

	PoolId addDescriptorBlueprint(const char* layoutName_, const char* blueprintName_)
	{
		return Globals::renderer.descriptorsManager.addDescriptorBlueprint(layoutName_, blueprintName_);
	}

	PoolId addMemberToDescriptorBlueprint(PoolId layoutId_, PoolId blueprintId_, const char* name_, DataTypeEnum dataType_)
	{
		return Globals::renderer.descriptorsManager.addMemberToDescriptorBlueprint(layoutId_, blueprintId_, name_, dataType_);
	}

	PoolId addDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_)
	{
		return Globals::renderer.descriptorsManager.addDescriptorSet(layoutId_, allocatorType_, memoryVisability_);
	}

	void removeDescriptorBlueprint(PoolId layoutId_, PoolId blueprintId_)
	{
		Globals::renderer.descriptorsManager.removeDescriptorBlueprint(layoutId_, blueprintId_);
	}

	void removeDescriptorSet(PoolId setId_)
	{
		Globals::renderer.descriptorsManager.removeDescriptorSet(setId_);
	}

	void removeDescriptorSetLayout(PoolId layoutId_)
	{
		Globals::renderer.descriptorsManager.removeDescriptorSetLayout(layoutId_);
	}

	PoolId addPipelineLayout()
	{
		return Globals::renderer.pipelinesManager.addPipelineLayout();
	}

	void addDescriptorSetLayoutToPipelineLayout(PoolId pipelineLayoutId_, PoolId descriptorSetLayoutId_)
	{
		Globals::renderer.pipelinesManager.addDescriptorSetLayoutToPipelineLayout(pipelineLayoutId_, descriptorSetLayoutId_);
	}

	PoolId addGraphicsPipelineLayout(const char* name_, PoolId pipelineLayoutId_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_)
	{
		return Globals::renderer.pipelinesManager.addGraphicsPipeline(name_, pipelineLayoutId_, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_, rRenderpass_, subpassId_);
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

    uint32_t addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_)
	{
		return Globals::renderer.passesGraph.addTaskToPass(passId_, name_, cmdBufferFunc_);
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
		VkCommandBufferBeginInfo beginInfo = {}; 
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
		vkBeginCommandBuffer(cmdBuffer_, &beginInfo);
		Globals::renderer.recordingPipelineId = UninitializedPoolId;
		Globals::renderer.recordingSetIds.clear();
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
		The set has to be compatible with the set layout at setIndex_ of the pipeline's layout (descriptor set layout compatibility), that's the only thing Vulkan requires for the bind. It is always bound, even while some of its descriptors have no data yet: every binding was written by DescriptorSet::create, and a shader only reads the bindings it uses.
		The set is remembered for the rest of the recording (Renderer::recordingSetIds), drawMeshIndexed looks up the descriptors its shaders declared (Shader::addDescriptor) in it.
	*/
	void bindDescriptorSet(PoolId descriptorSetId_, PoolId graphicsPipelineId_, uint32_t setIndex_, VkCommandBuffer &cmdBuffer_)
	{
		VkPipelineLayout pipelineLayout = getPipelinesManager().pipelineLayouts[getGraphicsPipeline(graphicsPipelineId_).pipelineLayoutId].get();
		if(Globals::renderer.recordingSetIds.size() <= setIndex_)
		{
			Globals::renderer.recordingSetIds.resize(setIndex_ + 1, UninitializedPoolId);
		}
		Globals::renderer.recordingSetIds[setIndex_] = descriptorSetId_;
		VkDescriptorSet descriptorSet = getDescriptorSet(descriptorSetId_).get();
		vkCmdBindDescriptorSets(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setIndex_, 1, &descriptorSet, 0, nullptr);
	}

	/* The set bound at setIndex_ in the command buffer being recorded. Throws when a shader declares a descriptor of a set index nothing was bound to, since the draw would read an unbound set. */
	static DescriptorSet& getRecordingSet(uint32_t setIndex_)
	{
		if(Globals::renderer.recordingSetIds.size() <= setIndex_ || Globals::renderer.recordingSetIds[setIndex_] == UninitializedPoolId)
		{
			throw std::runtime_error(std::format("Draw: A shader uses a descriptor of set index {}, but no set is bound there.", setIndex_));
		}
		return getDescriptorSet(Globals::renderer.recordingSetIds[setIndex_]);
	}

	/*
		A draw may only read the descriptors its shader declared once their first data was copied (DescriptorSet::isDescriptorSettled), later updates are ordered by the upload batch barriers. Descriptors the shader doesn't use don't matter, even when they share a set with one that is pending.
		A draw that fails this is skipped for the frame and recorded again on a later one (the command buffer is re-recorded every frame), like drawMeshIndexed skips until its uploads are in the GPU.
	*/
	static bool areShaderDescriptorsSettled(const Shader& rShader_)
	{
		for(const ShaderDescriptorUse& rUse : rShader_.descriptors)
		{
			if(!getRecordingSet(rUse.setIndex).isDescriptorSettled(rUse.descriptorId))
			{
				return false;
			}
		}
		return true;
	}

	/* Queued only for draws that are recorded, so a descriptor that was skipped keeps accepting its first writes. The entries are locked after the frame's submit (DescriptorsManager::lockBoundDescriptors). */
	static void queueShaderDescriptorLocks(const Shader& rShader_)
	{
		for(const ShaderDescriptorUse& rUse : rShader_.descriptors)
		{
			getDescriptorsManager().queueLockOnSubmit(Globals::renderer.recordingSetIds[rUse.setIndex], rUse.descriptorId);
		}
	}

	void drawMeshIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer &cmdBuffer_)
	{
		Mesh& rMesh = getMesh(meshId_);
		GraphicsPipeline& rPipeline = getGraphicsPipeline(Globals::renderer.recordingPipelineId);
		Shader& rVertexShader = getShader(rPipeline.vertexShaderId);
		Shader& rFragmentShader = getShader(rPipeline.fragmentShaderId);
		if(!areShaderDescriptorsSettled(rVertexShader) || !areShaderDescriptorsSettled(rFragmentShader))
		{
			return;
		}
		if(isUploadInGPU(rMesh.vbMemoryUploadId) && isUploadInGPU(rMesh.ibMemoryUploadId))
		{
			queueShaderDescriptorLocks(rVertexShader);
			queueShaderDescriptorLocks(rFragmentShader);

			const MemoryEntry& indexUpload = getMemoryEntry(rMesh.ibMemoryUploadId); 
			const MemoryEntry& vertexUpload = getMemoryEntry(rMesh.vbMemoryUploadId);
			MemoryManager& rMemoryManager = Globals::renderer.memoryManager;

			VkBuffer vertexBuffers[] = { rMemoryManager.getBuffer(vertexUpload).get() };
			VkDeviceSize vOffsets[] = { vertexUpload.inBufferFirstByte };
			vkCmdBindVertexBuffers(cmdBuffer_, 0, 1, vertexBuffers, vOffsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(cmdBuffer_, rMemoryManager.getBuffer(indexUpload).get(), indexUpload.inBufferFirstByte, VK_INDEX_TYPE_UINT32);
		
			// Draw indexed 
			vkCmdDrawIndexed(cmdBuffer_, getMesh(meshId_).indices.size(), instanceCount_, 0, 0, 0); 
		}
	}

	void endCMDsRecording(VkCommandBuffer &cmdBuffer_)
	{
		vkCmdEndRenderPass(cmdBuffer_);
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
