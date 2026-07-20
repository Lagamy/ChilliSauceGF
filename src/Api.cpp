#include "Api.h"
#include "Globals.h"
#include "Layout.h"
#include "PoolId.h"
#include "SubmissionBatch.h"
#include "UploadEntry.h"
#include "UploadId.h"
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
	
	DemoManager& getDemoManager()
	{
		return Globals::renderer.demoManager;
	}

	GPUMemoryManager& getGPUMemoryManager()
	{
		return Globals::renderer.gpuMemoryManager; 
	}

	PassesManager& getPassesManager()
	{
		return  Globals::renderer.passesManager;
	}

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(PoolId layoutId_)
	{
		return Globals::renderer.gpuPipelinesManager.graphicsPipelines.get(layoutId_); 
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
		return Globals::renderer.shadersManager.shaders.get(shaderId_); 
	}

	const UploadEntry& getUploadEntry(UploadId id_)
	{
		//if(id_allocatorType == STATIC)
		//{
			return Globals::renderer.gpuMemoryManager.staticAllocator.getUploadEntry(id_);
		//}

	}
	uint32_t getUploadStartingByteInGPUHeap(UploadId id_)
	{
		const UploadEntry& rEntry = getUploadEntry(id_); 
		return getGPUBufferOffset(STATIC, rEntry.bufferType) + rEntry.inBufferFirstByte; 
	}

	Buffer& getGPUBuffer(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_)
	{
		//if(allocatorType_ == STATIC)
		//{
			return Globals::renderer.gpuMemoryManager.staticAllocator.getBuffer(uploadType_);
		//}
	}
	
	uint32_t getGPUBufferOffset(AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_)
	{
		return  Globals::renderer.gpuMemoryManager.staticAllocator.gpuHeap.bufferOffsets[uploadType_]; 
	}

	uint32_t& getCurrentImageIndex()
	{
		return Globals::renderer.imageIndex; 
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
		return Globals::renderer.reflectionSystem.layoutsPerBufferType[VERTEX - 1].get(layoutId_); // - 1 due to INDEX layout not existing  
	}


	Mesh& getMesh(PoolId meshId_)
	{
		return Globals::renderer.resourcesManager.meshes.get(meshId_);
	} 
	
	void setFramesAtFlightCount(uint32_t count_)
	{
		Globals::renderer.framesAtFlightCount = count_;
	}


	// Add
	PoolId addSemaphore(const char* name_)
	{
		return Globals::renderer.syncManager.addSemaphore(name_);
	}
		
	PoolId addFence(const char* name_, bool createSignaled_)
	{
		return Globals::renderer.syncManager.addFence(name_, createSignaled_);
	}

	PoolId addShader(const char* name_, const char* path_)
	{
		return Globals::renderer.shadersManager.shaders.add(name_, Disk::executablePath + path_);  
	}

	void removeShader(PoolId& id_)
	{
		Globals::renderer.shadersManager.shaders.remove(id_); 
	}



	UploadId addUpload(const char* name_, AllocatorTypeEnum allocatorType_, BufferTypeEnum uploadType_, const void* data_, VkDeviceSize size_)
	{
		// if(allocatorType_ == STATIC)
		// {
		return Globals::renderer.gpuMemoryManager.staticAllocator.addUpload(name_, data_, size_, uploadType_);
		// }
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
		return Globals::renderer.gpuPipelinesManager.graphicsPipelines.add(name_, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_, rRenderpass_, subpassId_);
	} 

	void createAllPipelines()
	{
		Globals::renderer.gpuPipelinesManager.createAllPipelines(); 
	}

	void destroyAllPipelines()
	{
		Globals::renderer.gpuPipelinesManager.destroyAllPipelines();
	}
	
	PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_)
    {
		return addPass(name_, cmdType_, queueFamily_, signalFenceId_);
	}

	PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_)
	{
		return Globals::renderer.passesManager.addPass(name_, cmdType_, queueFamily_);
	} 

    uint32_t addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_)
	{
		return Globals::renderer.passesManager.addTaskToPass(passId_, name_, cmdBufferFunc_);
	}

	void addWaitSemaphoreToTask(PassId passsId_, uint32_t taskId_, PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_)
	{
		Globals::renderer.passesManager.getPass(passsId_).tasks[taskId_].addWaitSemaphore(waitSemaphoreId_, pipelineStage_);
	} 

	void addSignalSemaphoreToTask(PassId passsId_, uint32_t taskId_, PoolId signalSemaphoreId_)
	{
		Globals::renderer.passesManager.getPass(passsId_).tasks[taskId_].addSignalSemaphore(signalSemaphoreId_);
	}
	
	void enablePass(PassId passId_)
	{
		Globals::renderer.passesManager.enablePass(passId_); 
	} 

    void disablePass(PassId passId_) // Since oneshot - self disables
	{
		Globals::renderer.passesManager.disableFramePass(passId_);
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

	void bindMesh(PoolId meshId_, VkCommandBuffer &cmdBuffer_) 
	{
		Mesh& rMesh = getMesh(meshId_); 
		const UploadEntry& vertexUpload = getUploadEntry(rMesh.vbMemoryUploadId); 
		VkBuffer vertexBuffers[] = { getGPUBuffer(STATIC, VERTEX).get() };
		VkDeviceSize vOffsets[] = { vertexUpload.inBufferFirstByte }; 
		vkCmdBindVertexBuffers(cmdBuffer_, 0, 1, vertexBuffers, vOffsets); 

		// Bind index buffer 
		const UploadEntry& indexUpload = getUploadEntry(rMesh.ibMemoryUploadId); 
		vkCmdBindIndexBuffer(cmdBuffer_, getGPUBuffer(STATIC, INDEX).get(), indexUpload.inBufferFirstByte, VK_INDEX_TYPE_UINT32);
	}


	void drawIndexed(PoolId meshId_, uint32_t instanceCount_, VkCommandBuffer& cmdBuffer_) 
	{
		vkCmdDrawIndexed(cmdBuffer_, getMesh(meshId_).indices.size(), instanceCount_, 0, 0, 0); 
	}

	void endCMDsRecording(VkCommandBuffer &cmdBuffer_)
	{
		vkCmdEndRenderPass(cmdBuffer_);
		vkEndCommandBuffer(cmdBuffer_);
	}


	void submitPassesToQueues() // Note: Clear one shot passes submissions after submissions.
	{
		for(uint8_t i = 0; i < 3; i++)
		{
			Pool<SubmissionBatch>& rSubmissionBatches = getPassesManager().submissionBatchesPerQueue[i]; 
			for(uint32_t j = 0; j < rSubmissionBatches.size(); j++)
			{
				VkFence signalFence = VK_NULL_HANDLE; 
				if(rSubmissionBatches.objects[j].signalFenceId != UninitializedPoolId)
				{
					signalFence = getFence(rSubmissionBatches.objects[j].signalFenceId); 
				}

				vkQueueSubmit(getQueue(i), rSubmissionBatches.objects[j].submissions.size(), rSubmissionBatches.objects[j].submissions.data(), signalFence); 
				if(rSubmissionBatches.objects[j].oneShot) // disable cmdBuffers, and remove this submissionBatch from list 
				{
					for(const auto& rCmdBufferToDisable : rSubmissionBatches.objects[j].cmdBuffersToDisable)
					{ 
						CmdBuffersInPasses& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(i).commandBuffers; 
                    	rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rCmdBufferToDisable]); 
                    	rCommandBuffers.buffersToEnabled[rCmdBufferToDisable] = UninitializedId; 
					}
					rSubmissionBatches.removeInternal(j); 
				}
			}
		}
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
		VkSemaphore* pImageUseFinishedSemaphore = &getCurrentSwapchainImage().getInUseSemaphoreFinished();
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
