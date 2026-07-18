#include "Api.h"
#include "Globals.h"
#include "Layout.h"
#include "PoolId.h"
#include "SubmissionBatchId.h"
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
	GraphicsPipeline& getGraphicsPipeline(PoolId graphicsPipelineId_)
	{
		return Globals::renderer.graphicsPipelines.get(graphicsPipelineId_); 
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

	VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CmdTypeEnum poolType_, uint32_t id_)
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

	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CmdTypeEnum poolType_)
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
	
	SubmissionBatch& getSubmissionBatch(SubmissionBatchId batchId_)
	{
		return Globals::renderer.submitionManager.submissionBatches[batchId_.queueFamily].get(batchId_.poolId);
	}

	void setFramesAtFlightCount(uint32_t count_)
	{
		Globals::renderer.framesAtFlightCount = count_;
	}

	// Record 
	void recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_)
	{
		Globals::renderer.oneShotCommandPools.pools[queueFamily_].recordCmdBuffer(id_);
	}
		
	void recordCurrentFrameCmdPools()
	{
		for(auto& rFrameCmdPool : Globals::renderer.framesResources[Globals::renderer.currentFrame].frameCmdPools.pools)
		{
			rFrameCmdPool.recordCmdBuffers();
		}
	}

	// Reset 
	void resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_, Graphics::Fence& rFinishSignalFence_)
	{
		Globals::renderer.oneShotCommandPools.pools[queueFamily_].resetCmdBuffer(id_, rFinishSignalFence_);
	}
		
	void resetCurrentFrameCmdPools()
	{
		for(auto& rFrameCmdPool : Globals::renderer.framesResources[Globals::renderer.currentFrame].frameCmdPools.pools)
		{
			rFrameCmdPool.resetCmdPool();
		}
	}

	// Add
	PoolId addSemaphore(const char* name_)
	{
		return Globals::renderer.syncManager.addSemaphore(name_);
	}
		
	PoolId addFence(const char* name_, VkFenceCreateFlags flags_)
	{
		return Globals::renderer.syncManager.addFence(name_, flags_);
	}

	PoolId addShader(const char* name_, const char* path_)
	{
		return Globals::renderer.shadersManager.shaders.add(name_, Disk::executablePath + path_);  
	}

	void removeShader(PoolId& id_)
	{
		Globals::renderer.shadersManager.shaders.remove(id_); 
	}

	uint32_t addCmdBufferBlueprint(CMDTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_)
	{
		return Globals::renderer.demoManager.addCmdBufferBlueprint(poolType_, queueFamilyEnum_, commandsToRecord_);
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


	PoolId addGraphicsPipeline(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_)
	{
		return Globals::renderer.graphicsPipelines.add(name_, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_);
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


	void submitToPassesToQueues() // Note: Clear one shot passes submissions after submissions.
	{
		for(int i = 0; i < 3; i++)
		{
			for(const auto& rSubmissionBatch : getPassesManager().submissionBatchesPerQueue[i])
			{
				vkQueueSubmit(, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
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

		VkResult result = vkQueuePresentKHR(getMainDevice().queues.presentQueue, &presentInfo);
		if(result != VK_SUCCESS)
		{
			throw std::runtime_error("Presentation: Failed to present Image");
		} 
	}
}
