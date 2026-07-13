#include "Api.h"
#include "Globals.h"
#include "Layout.h"
#include "PoolId.h"
#include "UploadEntry.h"
#include "UploadId.h"
#include "Utilities.h"

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

	// For now i only need 1 of each
	GraphicsPipeline& getGraphicsPipeline(PoolId graphicsPipelineId_)
	{
		return Globals::renderer.graphicsPipelines.get(graphicsPipelineId_); 
	}

	RenderPass& getPresentationRenderPass()
	{
		return Globals::renderer.presentationRenderPass; 
	}

	CmdBufferBlueprintsPack& getCmdBufferBlueprints(CommandPoolTypeEnum poolType_)
	{
		return (poolType_ == FRAME ? Globals::renderer.demoManager.frameCmdBufferBlueprints : Globals::renderer.demoManager.oneShotCmdBufferBlueprints);
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

	VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_, uint32_t id_)
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

	const VkCommandPool& getCommandPool(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_)
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
	
	SubmitionBatch& getGraphicsSubmitionBatch(PoolId batchId_)
	{
		return Globals::renderer.submitionManager.graphicsSubmitionBatches.get(batchId_);
	}

	SubmitionBatch& getTransferSubmitionBatch(PoolId batchId_)
	{
		return Globals::renderer.submitionManager.transferSubmitionBatches.get(batchId_);
	}
	
	SubmitionBatch& getComputeSubmitionBatch(PoolId batchId_)
	{
		return Globals::renderer.submitionManager.computeSubmitionBatches.get(batchId_);
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

	uint32_t addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_)
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
	
	PoolId addGraphicsPipeline(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_); 
	
	PoolId addGraphicsSubmitionBatch(const char* name_)
	{
		return Globals::renderer.submitionManager.graphicsSubmitionBatches.add(name_);
	} 

	PoolId addTransferSubmitionBatch(const char* name_)
	{
		return Globals::renderer.submitionManager.transferSubmitionBatches.add(name_);
	} 

	PoolId addComputeSubmitionBatch(const char* name_)
	{
		return Globals::renderer.submitionManager.computeSubmitionBatches.add(name_);
	}
	
	PoolId addGraphicsSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_)
	{
		return Globals::renderer.submitionManager.addGraphicsSubmition(name_, batchId_, pCmdBuffer_, cmdBufferCount_, pWaitSemaphores_, waitSemaphoresCount_, pWaitSemToStages_, pSignalSemaphores_, signalSemaphoresCount_);
	} 
    
	PoolId addTransferSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_)
	{
		return Globals::renderer.submitionManager.addTransferSubmition(name_, batchId_, pCmdBuffer_, cmdBufferCount_, pWaitSemaphores_, waitSemaphoresCount_, pWaitSemToStages_, pSignalSemaphores_, signalSemaphoresCount_);
	} 

    PoolId addComputeSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_)
	{
		return Globals::renderer.submitionManager.addComputeSubmition(name_, batchId_, pCmdBuffer_, cmdBufferCount_, pWaitSemaphores_, waitSemaphoresCount_, pWaitSemToStages_, pSignalSemaphores_, signalSemaphoresCount_);
	} 
	
	void submitToGraphicsQueue(PoolId batchId_, VkFence signalFence_)
	{
		return Globals::renderer.submitionManager.submitToGraphicsQueue(batchId_, signalFence_);
	} 

    void submitToTransferQueue(PoolId batchId_, VkFence signalFence_)
	{
		return Globals::renderer.submitionManager.submitToTransferQueue(batchId_, signalFence_);
	} 

    void submitToComputeQueue(PoolId batchId_, VkFence signalFence_)
	{
		return Globals::renderer.submitionManager.submitToComputeQueue(batchId_, signalFence_);
	} 
   
	void windowSizeCallback(GLFWwindow*, int width, int height) 
	{
    	Globals::windowWidth = width;
    	Globals::windowHeight = height;
		// Globals::Mouse::lastX = width / 2.0f;
		// Globals::Mouse::lastY = height / 2.0f;
    	// glViewport(0, 0, width, height);
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
