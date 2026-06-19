namespace Graphics
{
// Get 
		Device& getMainDevice()
		{
			return Globals::renderer.mainDevice; 
		}

		Swapchain& getSwapchain()
		{
			return Globals::renderer.swapchain; 
		}
		// For now i only need 1 of each
		GraphicsPipeline& getGraphicsPipeline()
		{
			return Globals::renderer.graphicsPipeline; 
		}

		RenderPass& getRenderPass()
		{
			return Globals::renderer.renderpass; 
		}

		Semaphore& getSemaphore(PoolId semaphoreId_)
		{
			return Globals::renderer.syncManager.getSemaphore(semaphoreId_);
		}

		Fence& getFence(PoolId fenceId_)
		{
			return Globals::renderer.syncManager.getFence(fenceId_);
		}
	
		const VkCommandBuffer& getCommandBuffer(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_, uint32_t id_)
		{
			
			if(poolType_ == FRAME)
			{
				return Globals::renderer.framesResources[Globals::renderer.currentFrameAtFlight].frameCmdPools.getPoolByQueue(queueFamily_).commandBuffers.buffers[id_];
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
				return Globals::renderer.framesResources[Globals::renderer.currentFrameAtFlight].frameCmdPools.getPoolByQueue(queueFamily_).get();
			}
			else
			{
				return Globals::renderer.oneShotCommandPools.getPoolByQueue(queueFamily_).get();
			}
		}
	
		// Record 
		void recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_)
		{
			Globals::renderer.oneShotCommandPools.pools[queueFamily_].recordCmdBuffer(id_);
		}
		
		void recordCurrentFrameCmdPools()
		{
			for(auto& rFrameCmdPool : Globals::renderer.framesResources[Globals::renderer.currentFrameAtFlight].frameCmdPools.pools)
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
			for(auto& rFrameCmdPool : Globals::renderer.framesResources[Globals::renderer.currentFrameAtFlight].frameCmdPools.pools)
			{
				Fence& rCurrentFrameFinishFence = Globals::renderer.syncManager.getFence(Globals::renderer.framesResources[Globals::renderer.currentFrameAtFlight].frameFinishedFenceId);
				rFrameCmdPool.resetCmdPool(rCurrentFrameFinishFence);
			}
		}
	
		// Add
		PoolId addSemaphore(const char* name_)
		{
			Globals::renderer.syncManager.addSemaphore(name_);
		}
		
		PoolId addFence(const char* name_)
		{
			Globals::renderer.syncManager.addFence(name_);
		}

		uint32_t addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_)
		{
			Globals::renderer.demoManager.addCmdBufferBlueprint(poolType_, queueFamilyEnum_, commandsToRecord_);
		}

		uint32_t addUpload(const char* name_, AllocatorTypeEnum allocatorType_, const void* data_, VkDeviceSize size_, UploadTypeEnum uploadType_)
		{
			if(allocatorType_ == STATIC)
			{
				Globals::renderer.gpuMemoryManager.staticAllocator.addUpload(name_, data_, size_, uploadType_);
			}
		}
}
