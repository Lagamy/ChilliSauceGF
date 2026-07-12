#include "Triangle.h"
#include "Api.h"
#include "Layout.h"
#include "Utilities.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
	Triangle::Triangle()
	{
		DemoManager& rDemoManager = getDemoManager(); 
		rDemoManager.loadDemo = [this]() { this->load(); }; 
		rDemoManager.createGPUObjects = [this]() {this->createGPUObjects(); }; 
		rDemoManager.submitToGPU = [this]() { this->submit(); }; 
	}


	void Triangle::load()
	{

		//std::cout << "path: " << std::string(Disk::getExecutablePath() + "/Assets/shaders/triangle/vert.spv") << std::endl;
		this->vertexShaderId = addShader("Triangle Vertex Shader", "Assets/shaders/triangle/vert.spv"); 
		this->fragmentShaderId = addShader("Triangle Fragment Shader", "Assets/shaders/triangle/frag.spv"); 
		RenderPass& rPresentationRenderPass = getPresentationRenderPass();
	
		
		this->verticeLayoutId = addVerticeLayout("Triangle Vertice Layout");
		PoolId positionId = addMemberToVerticeLayout(this->verticeLayoutId, "Position", VEC3);
		PoolId colorId = addMemberToVerticeLayout(this->verticeLayoutId, "Color", VEC4); 


		this->meshId = addMesh("Triangle", this->verticeLayoutId, 3);
		Mesh& rMesh = getMesh(this->meshId); 
		rMesh.setVerticeMember(positionId, 0, glm::vec3(0.0,-0.5, 0.0));
		rMesh.setVerticeMember(positionId, 1, glm::vec3(0.5, 0.5, 0.0));
		rMesh.setVerticeMember(positionId, 2, glm::vec3(-0.5, 0.5, 0.0));

		rMesh.setVerticeMember(colorId, 0, glm::vec4(1.0, 0.0, 0.0, 1.0)); 
		rMesh.setVerticeMember(colorId, 1, glm::vec4(0.0, 1.0, 0.0, 1.0));
		rMesh.setVerticeMember(colorId, 2, glm::vec4(0.0, 0.0, 1.0, 1.0));
		
		rMesh.indices = { 
		 	0, 1, 2
		};
		
		/**************/

		/* Upload Mesh */
		rMesh.queueGPUUpload(); 
		/**************/
	
		/* Configure RenderPass*/
		rPresentationRenderPass.addColorAttachment(getSwapchain().imageFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,  VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR); // STORE_OP_DONT_CARE - means we dont care what will happen to the attachment after reading it
	
		SubPassDescriptionInfo subpassDescription = {}; 
		subpassDescription.pRenderPass = &rPresentationRenderPass; 
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
		subpassDescription.colorAttachmentsToUseIds = {0};

		SubPassLayoutTransitionInfo subpassLayoutTransition = {};
		subpassLayoutTransition.stageMaskFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
		subpassLayoutTransition.accessMaskFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassLayoutTransition.dependencyFlags = 0; 

		rPresentationRenderPass.addSubpass(subpassDescription, subpassLayoutTransition); 

		/* Create Graphics Pipeline */

		this->graphicsPipelineId = addGraphicsPipeline("Draw Triangle", this->vertexShaderId, this->fragmentShaderId, rMesh.verticeLayoutId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);

		/* Initialize Command Buffer Blueprints */
		this->cmdBufferId = addCmdBufferBlueprint(
			FRAME,
			GRAPHICS, 
			[this](VkCommandBuffer& cmd) { recordCMDs(cmd); } 
		);
	}

	void Triangle::createGPUObjects()
	{
		getPresentationRenderPass().create(); 
		getGraphicsPipeline(this->graphicsPipelineId).create(getPresentationRenderPass(), 0);
	}

	void Triangle::recordCMDs(VkCommandBuffer& cmdBuffer_)
	{
		VkCommandBufferBeginInfo beginInfo = {}; 
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
		vkBeginCommandBuffer(cmdBuffer_, &beginInfo);
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
		
		// Bind graphics pipeline  
		vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, getGraphicsPipeline(this->graphicsPipelineId).get());
		
		// Bind vertex buffer 
		Mesh& rMesh = getMesh(this->meshId); 
		const UploadEntry& vertexUpload = getUploadEntry(rMesh.vbMemoryId); 
		VkBuffer vertexBuffers[] = { getGPUBuffer(STATIC, VERTEX).get() };
		VkDeviceSize vOffsets[] = { 0 }; 
		vkCmdBindVertexBuffers(cmdBuffer_, 0, 1, vertexBuffers, vOffsets); 

		// Bind index buffer 
		const UploadEntry& indexUpload = getUploadEntry(rMesh.ibMemoryId); 
		vkCmdBindIndexBuffer(cmdBuffer_, getGPUBuffer(STATIC, INDEX).get(), indexUpload.inBufferFirstByte, VK_INDEX_TYPE_UINT32);
	
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
		// printf("Drawing triangle\n");
		// vkCmdDrawIndexed(cmdBuffer_, 3, 1, 0, 0, 0);
		vkCmdDraw(cmdBuffer_, 3, 1,0,0);
		vkCmdEndRenderPass(cmdBuffer_);
		vkEndCommandBuffer(cmdBuffer_);
	}

	void Triangle::submit() 
	{
		VkSemaphore* pImageUseFinishedSemaphore = &getSemaphore(getSwapchain().imageUseFinishedSemaphoreIds[getCurrentImageIndex()]);
		PoolId batchId = addGraphicsSubmitionBatch("Render Triangle"); 
		VkCommandBuffer cmdBuffer =  getCommandBuffer(GRAPHICS, FRAME, this->cmdBufferId); 
		// Render Frame 
		if(this->firstFrame)
		{
			std::array<VkSemaphore, 2> waitSemaphores = {
				getSemaphore(getCurrentFrameResources().imageAvailableSemaphoreId),
				getSemaphore(getGPUMemoryManager().staticUploadFinishedSemaphoreId)
			}; 
			
			std::array<VkPipelineStageFlags, waitSemaphores.size()> waitStages = { 
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // frameAvailable
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT // staticUploadSemaphore
			}; 
			
			addGraphicsSubmition("Triangle Submit", batchId, &cmdBuffer, 1, waitSemaphores.data(), waitSemaphores.size(), waitStages.data(), pImageUseFinishedSemaphore, 1); 
		}
		else 
		{	
			VkSemaphore imageAvailableSemaphore = getSemaphore(getCurrentFrameResources().imageAvailableSemaphoreId); 
			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  
			addGraphicsSubmition("Triangle Submit", batchId, &cmdBuffer, 1, &imageAvailableSemaphore, 1, &waitStage, pImageUseFinishedSemaphore, 1); 
		}
		submitToGraphicsQueue(batchId, getFence(getCurrentFrameResources().frameAvailableFenceId)); 
		// Present Frame 
		presentToScreen(); 
		this->firstFrame = false; 
	}
}
