#include "Triangle.h"
#include "Globals.h"
#include "GraphicsPipeline.h"
#include <vulkan/vulkan_core.h>

void Triangle::load()
{
	Shader vertexShader = Shader("Assets/shaders/triangle");
	Shader fragmentShader = Shader("Assets/shaders/triangle");
	RenderPass& rRenderPass = Demo::renderer.renderpass;
	GraphicsPipeline& rGraphicsPipeline = Demo::renderer.graphicsPipeline;
	
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
	mesh.vbMemoryId = Demo::renderer.gpuMemoryManager.addEntry("Triangle", sizeof(Vertex) * mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, false); 
	Demo::renderer.gpuMemoryManager.upload(mesh.vbMemoryId, mesh.vertices.data()); 
		
	// Create Index Buffer and fill it with data.
	mesh.ibMemoryId = Demo::renderer.gpuMemoryManager.addEntry("Triangle", sizeof(mesh.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, false); 
	Demo::renderer.gpuMemoryManager.upload(mesh.ibMemoryId, mesh.indices.data());
	/**************/
	
	/* Configure RenderPass*/
	rRenderPass.addColorAttachment(Demo::renderer.swapchain.imageFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR); // STORE_OP_DONT_CARE - means we dont care what will happen to the attachment after reading it
	
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
	Demo::renderer.renderFlow.addCmdBufferBlueprint(
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
	renderPassInfo.renderPass = Demo::renderer.renderpass.get(); 
	renderPassInfo.framebuffer = Demo::renderer.swapchain.framebuffers[Demo::renderer.currentFrameAtFlight].get(); 
	renderPassInfo.renderArea = { 
		.offset = {0, 0}, 
		.extent = Demo::renderer.swapchain.extent
	};
	renderPassInfo.clearValueCount = 1; 
	renderPassInfo.pClearValues = &clearColor; 

	vkCmdBeginRenderPass(cmdBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // VK_SUBPASS_CONTENTS_INLINE - mean The commands for this subpass will be recorded directly into this primary command buffer.
	
	// Bind graphics pipeline  
	vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, Demo::renderer.graphicsPipeline.get());
	
	// Bind vertex buffer 
	VkBuffer vertexBuffers[] = { Demo::renderer.gpuMemoryManager.getEntry(this->mesh.vbMemoryId).buffer.get() }; 
	VkDeviceSize vOffsets[] = { 0 }; 
	vkCmdBindVertexBuffers(cmdBuffer_, 0, 1, vertexBuffers, vOffsets); 
	
	// Bind index buffer 
	vkCmdBindIndexBuffer(cmdBuffer_, Demo::renderer.gpuMemoryManager.getEntry(this->mesh.ibMemoryId).buffer.get(), 0, VK_INDEX_TYPE_UINT32);
	
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
	vkCmdDrawIndexed(cmdBuffer_, mesh.indices.size(), 1, 0, 0, 0);

	vkEndCommandBuffer(cmdBuffer_);
}

