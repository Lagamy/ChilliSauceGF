#include "Triangle.h"
#include "Globals.h"
#include "GraphicsPipeline.h"
#include <vulkan/vulkan_core.h>

Triangle::Triangle()
{
	Shader vertexShader = Shader("");
	Shader fragmentShader = Shader("");
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
	mesh.vbMemoryId = Demo::GPUMemoryManager.addEntry("Triangle", sizeof(Vertex) * mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, false); 
	Demo::GPUMemoryManager.upload(mesh.vbMemoryId, mesh.vertices.data(), Demo::renderer.mainDevice.queues.graphicsQueue); 
		
	// Create Index Buffer and fill it with data.
	mesh.ibMemoryId = Demo::GPUMemoryManager.addEntry("Triangle", sizeof(mesh.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, false); 
	Demo::GPUMemoryManager.upload(mesh.ibMemoryId, mesh.indices.data(), Demo::renderer.mainDevice.queues.graphicsQueue);
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
	
}
