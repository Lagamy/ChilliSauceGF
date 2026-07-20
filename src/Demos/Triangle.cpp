#include "Triangle.h"
#include "Api.h"
#include "SubmissionBatchId.h"
#include "Utilities.h"
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
		rMesh.queueStaticGPUUpload(); 
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

		this->graphicsPipelineId = addGraphicsPipelineLayout("Draw Triangle", this->vertexShaderId, this->fragmentShaderId, rMesh.verticeLayoutId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);

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
		beginCMDsRecording(cmdBuffer_); 
		
		// Bind graphics pipeline  
		bindGraphicsPipeline(this->graphicsPipelineId, cmdBuffer_); 

		// Bind vertex buffer 
		bindMesh(meshId, cmdBuffer_); 

		// Draw
		drawIndexed(this->meshId, 1, cmdBuffer_);
		endCMDsRecording(cmdBuffer_); 
	}

	void Triangle::submit() 
	{
		VkCommandBuffer cmdBuffer =  getCommandBuffer(GRAPHICS, FRAME, this->cmdBufferId); 
		SubmissionBatchId batchId = addSubmissionBatch("Render Triangle", GRAPHICS); 
		// Render Frame 
		PoolId submitionId = addSubmission("Triangle Pass", batchId, &cmdBuffer, 1);
		addWaitSemaphoreToSubmission(batchId, submitionId, getCurrentFrameResources().imageAcquireSemaphore.get(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		if(this->firstFrame)
		{
			addWaitSemaphoreToSubmission(batchId, submitionId, getGPUMemoryManager().staticUploadFinishedSemaphore.get(), VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
		}
		addSignalSemaphoreToSubmission(batchId, submitionId, getCurrentSwapchainImage().getInUseSemaphoreFinished());
		submitToGraphicsQueue(batchId, getCurrentFrameResources().frameAvailableFence.get()); 
		// Present Frame 
		presentToScreen(); 
		this->firstFrame = false; 
	}
}
