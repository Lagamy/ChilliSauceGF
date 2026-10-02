#include "Triangle.h"
#include "Api.h"
#include "Utilities.h"
#include <glm/ext/vector_float4.hpp>
#include <limits>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
	namespace Triangle
	{		
		void setGPUSceneToTriangle()
		{
			GPUSceneManager& rGPUSceneManager = getGPUSceneManager();
			setEnvironmentSetupFunction(setupEnvironment); 
			rGPUSceneManager.changeGPUScene(defineLayouts, defineResources, definePasses, EmptyFunction, updateResources, EmptyFunction, EmptyFunction);
		}

		void setupEnvironment()
		{
			setMaxDormantUpdateStagingHeaps(5); 
			addPageForDynamicMemoryEntries(CPU_SHARED, 10, MB, 10, MB, 5); 
			addPageForDynamicMemoryEntries(GPU_LOCAL, 10, MB, 10, MB, 5); 
		}

		void defineLayouts()
		{
			vertexShaderId = addShader("Triangle Vertex Shader", "Assets/shaders/triangle/vert.spv"); 
			fragmentShaderId = addShader("Triangle Fragment Shader", "Assets/shaders/triangle/frag.spv");

			vertexLayoutId = addVerticeLayout("Triangle Vertice Layout");
			vertexMemberIds.positionId = addMemberToVerticeLayout(vertexLayoutId, "Position", VEC3);
			vertexMemberIds.colorId = addMemberToVerticeLayout(vertexLayoutId, "Color", VEC4);
			graphicsPipelineId = addGraphicsPipelineLayout("Draw Triangle", vertexShaderId, fragmentShaderId, vertexLayoutId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, getPresentationRenderPass(), 0);
		
		}

		void defineResources()
		{
			meshId = addMesh("Triangle", vertexLayoutId, 3);
			Mesh& rMesh = getMesh(meshId); 
			/* You could do that, if you are willing to let user change materials or vertex data from Scripting with lua for example(In which case - you can't save PoolId from layout creation) */
			// PoolId positionMemberId = getVerticeLayout(this->vertexLayoutIds.layoutId).getMemberIdFromName("Position"); 
			// PoolId colorMemberId = getVerticeLayout(this->vertexLayoutIds.layoutId).getMemberIdFromName("Color"); 
			rMesh.setVerticeMember(vertexMemberIds.positionId, 0, glm::vec3(0.0,-0.5, 0.0));
			rMesh.setVerticeMember(vertexMemberIds.positionId, 1, glm::vec3(0.5, 0.5, 0.0));
			rMesh.setVerticeMember(vertexMemberIds.positionId, 2, glm::vec3(-0.5, 0.5, 0.0));

			rMesh.setVerticeMember(vertexMemberIds.colorId, 0, glm::vec4(1.0, 0.0, 0.0, 1.0)); 
			rMesh.setVerticeMember(vertexMemberIds.colorId, 1, glm::vec4(0.0, 1.0, 0.0, 1.0));
			rMesh.setVerticeMember(vertexMemberIds.colorId, 2, glm::vec4(0.0, 0.0, 1.0, 1.0));
		
			rMesh.indices = { 
		 		0, 1, 2
			};
		
			/* Upload Mesh */
			rMesh.queueUpload(DYNAMIC, GPU_LOCAL); 
		}

		void definePasses()
		{
			PassId passId = addPass("Triangle Pass", FRAME, GRAPHICS, getCurrentFrameAvailableFence);
			uint32_t taskId = addTaskToPass(passId, "Draw Triangle", recordCMDs); 
		
			addDynamicWaitSemaphoreToTask(passId, taskId, getCurrentFrameImageIsAcquiredSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			addDynamicSignalSemaphoreToTask(passId, taskId, getCurrentSwapchainImageUseFinishedSemaphore);
			enablePass(passId); 
		}

		void updateResources()
		{
			counter++; 
			if(counter == 7000)
			{
				Mesh& rMesh = getMesh(meshId);
				rMesh.updateVerticeMember(vertexMemberIds.colorId, 0, glm::vec4(0.0, 0.5, 0.5, 1.0)); 			
				rMesh.updateVerticeMember(vertexMemberIds.colorId, 1, glm::vec4(0.5, 0.0, 0.5, 1.0)); 			
				rMesh.updateVerticeMember(vertexMemberIds.colorId, 2, glm::vec4(0.5, 0.5, 0.0, 1.0)); 			
				rMesh.queueUpdate(); 
			}
			
			if (counter > 14000)
			{
				counter = 0; 
				Mesh& rMesh = getMesh(meshId);
				rMesh.updateVerticeMember(vertexMemberIds.colorId, 0, glm::vec4(1.0, 0.0, 0.0, 1.0)); 
				rMesh.updateVerticeMember(vertexMemberIds.colorId, 1, glm::vec4(0.0, 1.0, 0.0, 1.0));
				rMesh.updateVerticeMember(vertexMemberIds.colorId, 2, glm::vec4(0.0, 0.0, 1.0, 1.0));
				rMesh.queueUpdate();
			}
		}

		void recordCMDs(VkCommandBuffer& cmdBuffer_)
		{
			beginCMDsRecording(cmdBuffer_); 
			// Bind graphics pipeline  
			bindGraphicsPipeline(graphicsPipelineId, cmdBuffer_); 
			// Bind and draw vertex buffer 
			drawMeshIndexed(meshId, 1, cmdBuffer_); 
			endCMDsRecording(cmdBuffer_); 
		}
	}
}
