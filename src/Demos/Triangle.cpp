#include "Triangle.h"
#include "Api.h"
#include "Utilities.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
	void Triangle::load()
	{
		Shader vertexShader = Shader("Assets/shaders/triangle");
		Shader fragmentShader = Shader("Assets/shaders/triangle");
		RenderPass& rRenderPass = getRenderPass();
		GraphicsPipeline& rGraphicsPipeline = getGraphicsPipeline();
	
		/* Init Mesh */
		mesh.vertices = {
			glm::vec3(-0.5, -0.5, 0.0), 
			glm::vec3(0.5, -0.5, 0.0), 
			glm::vec3(0.0, 0.5, 0.0) 
		};

		mesh.indices = { 
			0, 2, 1
		};
		/**************/

		/* Upload Mesh */
		mesh.vbMemoryId = addUpload("Triangle Vertices", STATIC, mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size(), VERTEX); 
		
		// Create Index Buffer and fill it with data.
		mesh.ibMemoryId = addUpload("Triangle Indices", STATIC, mesh.indices.data(), sizeof(uint32_t) * mesh.indices.size(), INDEX); 
		/**************/
	
		/* Configure RenderPass*/
		rRenderPass.addColorAttachment(getSwapchain().imageFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR); // STORE_OP_DONT_CARE - means we dont care what will happen to the attachment after reading it
	
		SubPassDescriptionInfo subpassDescription = {}; 
		subpassDescription.pRenderPass = &rRenderPass; 
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
		subpassDescription.colorAttachmentsToUseIds = {0};

		SubPassLayoutTransitionInfo subpassLayoutTransition = {};
		subpassLayoutTransition.stageMaskFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
		subpassLayoutTransition.accessMaskFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassLayoutTransition.dependencyFlags = 0; 

		rRenderPass.addSubpass(subpassDescription, subpassLayoutTransition); 

		/* Configure Graphics Pipeline */
		rGraphicsPipeline = GraphicsPipeline(vertexShader, fragmentShader, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);

		/* Initialize Command Buffer Blueprints */
		this->cmdBufferId = addCmdBufferBlueprint(
			FRAME,
			GRAPHICS, 
			[this](VkCommandBuffer& cmd) { recordCMDs(cmd); } 
		);
	}

	void Triangle::recordCMDs(VkCommandBuffer& cmdBuffer_)
	{
		VkCommandBufferBeginInfo beginInfo = {}; 
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
		vkBeginCommandBuffer(cmdBuffer_, &beginInfo);
		VkClearValue clearColor = {};
		clearColor.color = {0.0f, 0.0f, 0.0f, 1.0f};

		VkRenderPassBeginInfo renderPassInfo = {}; 
		renderPassInfo.renderPass = getRenderPass().get(); 
		renderPassInfo.framebuffer = getSwapchain().framebuffers[getCurrentImageIndex()].get(); 
		renderPassInfo.renderArea = { 
			.offset = {0, 0}, 
			.extent = getSwapchain().extent
		};
		renderPassInfo.clearValueCount = 1; 
		renderPassInfo.pClearValues = &clearColor; 
	
		vkCmdBeginRenderPass(cmdBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // VK_SUBPASS_CONTENTS_INLINE - mean The commands for this subpass will be recorded directly into this primary command buffer.
		
		// Bind graphics pipeline  
		vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, getGraphicsPipeline().get());
		
		// Bind vertex buffer 
		const UploadEntry& vertexUpload = getUploadEntry(this->mesh.vbMemoryId, STATIC, VERTEX); 
		VkBuffer vertexBuffers[] = { getUploadHeapBuffer(STATIC, VERTEX).get() };
		VkDeviceSize vOffsets[] = { vertexUpload.heapStartingByte }; 
		vkCmdBindVertexBuffers(cmdBuffer_, 0, 1, vertexBuffers, vOffsets); 

		// Bind index buffer 
		const UploadEntry& indexUpload = getUploadEntry(this->mesh.ibMemoryId, STATIC, INDEX); 
		vkCmdBindIndexBuffer(cmdBuffer_, getUploadHeapBuffer(STATIC, INDEX).get(), indexUpload.heapStartingByte, VK_INDEX_TYPE_UINT32);
	
		// Viewport and Scissor (for dynamic) 
		// VkViewport viewport = {}; 
		// viewport.x = 0.0f; 
		// viewport.y = 0.0f; 
		// viewport.width = (float)Demo::renderer.swapchain.extent.width;
		// viewport.height = (float)Demo::renderer.swapchain.extent.height;
		// vkCmdSetViewport(cmdBuffer_, 0, 1, &viewport);

		// VkRect2D scissor = {}; 
		// scissor.offset = { 0, 0 };
		// scissor.extent = Demo::renderer.swapchain.extent;

		// Draw
		vkCmdDrawIndexed(cmdBuffer_, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
	
		vkEndCommandBuffer(cmdBuffer_);
	}

	void Triangle::submit() 
	{
		VkSemaphore* pFrameFinishedSemaphore = &getSemaphore(getCurrentFrameResources().frameFinishedSemaphoreId).vkHandle;
		
		// Render Frame 
		VkSubmitInfo submitInfo = {}; 
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 
		
		VkSemaphore waitSemaphores[] = {
			getSemaphore(getCurrentFrameResources().frameAvailableSemaphoreId).vkHandle,
			getSemaphore(getGPUMemoryManager().staticUploadFinishedSemaphoreId).vkHandle
		}; 

		submitInfo.waitSemaphoreCount = 2; 
		submitInfo.pWaitSemaphores = waitSemaphores; 
		VkPipelineStageFlags waitStages[] = { 
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		}; 
		submitInfo.pWaitDstStageMask = waitStages; 
		submitInfo.commandBufferCount = 1; 
		submitInfo.pCommandBuffers = &getCommandBuffer(GRAPHICS, FRAME, this->cmdBufferId);
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = pFrameFinishedSemaphore; 
		VkResult result = vkQueueSubmit(getMainDevice().queues.graphicsQueue, 1, &submitInfo, getFence(getCurrentFrameResources().frameAvailableFenceId).vkHandle); // Signal it -> to notify that we can submit this frame
		if(result != VK_SUCCESS)
		{
			throw std::runtime_error("Triangle: Failed to submit renderFrame cmdBuffer to the Graphics Queue"); 
		}

		// Present Frame 
		VkPresentInfoKHR presentInfo = {}; 
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; 
		presentInfo.waitSemaphoreCount = 1; 
		presentInfo.pWaitSemaphores = pFrameFinishedSemaphore; 
		presentInfo.pSwapchains = &getSwapchain().vkHandle; 
		presentInfo.pImageIndices = &getCurrentImageIndex();

		result = vkQueuePresentKHR(getMainDevice().queues.presentQueue, &presentInfo);
		if(result != VK_SUCCESS)
		{
			throw std::runtime_error("Triangle: Failed to present Image");
		}
	}
}
