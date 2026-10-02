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

	PoolId getCurrentFrameAvailableFence()
	{
		return getCurrentFrameResources().frameAvailableFenceId;
	}

	uint64_t getIndividualUpdateMemBlockSize()
	{
		return Globals::individualUpdateMemBlockSize;
	} 

	uint32_t getMaxDormantUpdateStagingHeaps()
	{
		return Globals::maxDormantUpdateStagingHeaps;
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
		return  Globals::renderer.memoryManager.staticAllocator.gpuHeap.bufferFirstByte[uploadType_]; 
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
		return Globals::renderer.reflectionSystem.layoutsPerBufferType[VERTEX - 1][layoutId_]; // - 1 due to INDEX layout not existing  
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

	void setMaxDormantUpdateStagingHeaps(uint32_t value_)
	{
		Globals::maxDormantUpdateStagingHeaps = value_; 
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

	void removeShader(PoolId& id_)
	{
		Globals::renderer.shadersManager.shaders.remove(id_); 
	}



	MemoryEntryId addAndUploadMemoryEntry(const char* name_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_, const void* data_, VkDeviceSize size_)
	{
		return Globals::renderer.memoryManager.addEntry(name_, data_, size_, allocatorType_, memoryVisability_, uploadType_);
	}

	void updateMemoryEntry(MemoryEntryId uploadId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
	{
		Globals::renderer.memoryManager.updateEntry(uploadId_, data_, inSrcOffset_, inEntryOffset_, size_);
	}

	PoolId addVerticeLayout(const char* name_)
	{
		return Globals::renderer.reflectionSystem.layoutsPerBufferType[VERTEX - 1].add(name_); 
	}

	PoolId addMemberToVerticeLayout(PoolId layoutId_, const char* name_, DataTypeEnum dataType_)
	{
		return getVerticeLayout(layoutId_).addMemberBlueprint(name_, dataType_);
	}

	PoolId addMesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_)
	{
		return Globals::renderer.resourcesManager.meshes.add(name_, name_, verticeLayoutId_, repeatCount_); 
	} 

	PoolId addGraphicsPipelineLayout(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_)
	{
		return Globals::renderer.pipelinesManager.graphicsPipelines.add(name_, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_, rRenderpass_, subpassId_);
	} 

	void addPageForDynamicMemoryEntries(MemoryVisabilityEnum memoryVisability_, uint32_t upperBoundForEntrySize_, StorageUnitEnum upperBoundUnit_, uint32_t memoryBlockSize_, StorageUnitEnum memoryBlockSizeUnit_, uint32_t maxDormantStagingHeaps_)
	{
		getMemoryManager().dynamicAllocator.addPage(memoryVisability_, upperBoundForEntrySize_, upperBoundUnit_, memoryBlockSize_, memoryBlockSizeUnit_, maxDormantStagingHeaps_);
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
		vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, getGraphicsPipeline(graphicsPipelineId_).get());
	}

	void drawMeshIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer &cmdBuffer_) 
	{
		Mesh& rMesh = getMesh(meshId_); 
		if(isUploadInGPU(rMesh.vbMemoryUploadId) && isUploadInGPU(rMesh.ibMemoryUploadId))
		{
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
