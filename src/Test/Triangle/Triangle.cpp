#include "Triangle.h"
#include "Globals.h"
#include "GraphicsPipeline.h"

Triangle::Triangle()
{
	Shader vertexShader = Shader("");
	Shader fragmentShader = Shader("");
	RenderPass& rRenderPass = Demo::renderer.renderpass;
	GraphicsPipeline& rGraphicsPipeline = Demo::renderer.graphicsPipeline;
	
	rGraphicsPipeline = GraphicsPipeline(vertexShader, fragmentShader. )

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
	
	/* Init RenderPass*/

	/* Init Graphics Pipeline */
	rGraphicsPipeline = GraphicsPipeline(vertexShader, fragmentShader, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
	rGraphicsPipeline.create(rRenderPass, 0);
}
