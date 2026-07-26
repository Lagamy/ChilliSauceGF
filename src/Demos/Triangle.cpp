#include "Triangle.h"
#include "Api.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
	Triangle::Triangle()
	{
		DemoManager& rDemoManager = getDemoManager(); 
		rDemoManager.defineLayouts = [this]() { this->defineLayouts(); }; 
		rDemoManager.defineResources = [this]() {this->defineResources(); }; 
		rDemoManager.definePasses = [this]() { this->definePasses(); }; 
	}

	void Triangle::defineLayouts()
	{
		this->vertexShaderId = addShader("Triangle Vertex Shader", "Assets/shaders/triangle/vert.spv"); 
		this->fragmentShaderId = addShader("Triangle Fragment Shader", "Assets/shaders/triangle/frag.spv");

		this->vertexLayoutId = addVerticeLayout("Triangle Vertice Layout");
		this->vertexMemberIds.positionId = addMemberToVerticeLayout(this->vertexLayoutId, "Position", VEC3);
		this->vertexMemberIds.colorId = addMemberToVerticeLayout(this->vertexLayoutId, "Color", VEC4);
		this->graphicsPipelineId = addGraphicsPipelineLayout("Draw Triangle", this->vertexShaderId, this->fragmentShaderId, this->vertexLayoutId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, getPresentationRenderPass(), 0);
		
	}

	void Triangle::defineResources()
	{
		this->meshId = addMesh("Triangle", this->vertexLayoutId, 3);
		Mesh& rMesh = getMesh(this->meshId); 
		/* You could do that, if you are willing to let user change materials or vertex data from Scripting with lua for example(In which case - you can't save PoolId from layout creation) */
		// PoolId positionMemberId = getVerticeLayout(this->vertexLayoutIds.layoutId).getMemberIdFromName("Position"); 
		// PoolId colorMemberId = getVerticeLayout(this->vertexLayoutIds.layoutId).getMemberIdFromName("Color"); 
		rMesh.setVerticeMember(this->vertexMemberIds.positionId, 0, glm::vec3(0.0,-0.5, 0.0));
		rMesh.setVerticeMember(this->vertexMemberIds.positionId, 1, glm::vec3(0.5, 0.5, 0.0));
		rMesh.setVerticeMember(this->vertexMemberIds.positionId, 2, glm::vec3(-0.5, 0.5, 0.0));

		rMesh.setVerticeMember(this->vertexMemberIds.colorId, 0, glm::vec4(1.0, 0.0, 0.0, 1.0)); 
		rMesh.setVerticeMember(this->vertexMemberIds.colorId, 1, glm::vec4(0.0, 1.0, 0.0, 1.0));
		rMesh.setVerticeMember(this->vertexMemberIds.colorId, 2, glm::vec4(0.0, 0.0, 1.0, 1.0));
		
		rMesh.indices = { 
		 	0, 1, 2
		};
		
		/* Upload Mesh */
		rMesh.queueStaticGPUUpload(); 
	} 

	void Triangle::definePasses()
	{
		PassId passId = addPass("Triangle Pass", FRAME, GRAPHICS, [](){return getCurrentFrameResources().frameAvailableFenceId; }); 
		uint32_t taskId = addTaskToPass(passId, "Draw Triangle", [this](VkCommandBuffer& cmd) { recordCMDs(cmd); }); 
		
		addDynamicWaitSemaphoreToTask(passId, taskId, [](){return getCurrentFrameResources().imageAcquiredSemaphoreId;}, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		addDynamicSignalSemaphoreToTask(passId, taskId, []() {return getCurrentSwapchainImage().imageInUseSemaphoreFinishedId;});
		enablePass(passId); 
	}

	void Triangle::recordCMDs(VkCommandBuffer& cmdBuffer_)
	{
		beginCMDsRecording(cmdBuffer_); 
		// Bind graphics pipeline  
		bindGraphicsPipeline(this->graphicsPipelineId, cmdBuffer_); 
		// Bind and draw vertex buffer 
		drawMeshIndexed(meshId, 1, cmdBuffer_); 
		endCMDsRecording(cmdBuffer_); 
	}

}
