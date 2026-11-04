#include "Triangle.h"
#include "Api.h"
#include "DescriptorSet.h"
#include "Utilities.h"
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
			addPageForStagingUploads(64, KB, 1, MB, 2); 
			addPageForStagingUploads(10, MB, 10, MB, 2); 
			addPageForDynamicMemoryEntries(CPU_SHARED, 10, MB, 10, MB); 
			addPageForDynamicMemoryEntries(GPU_LOCAL, 10, MB, 10, MB); 
		}

		void defineLayouts()
		{
			vertexShaderId = addShader("Triangle Vertex Shader", "Assets/shaders/triangle/vert.spv"); 
			fragmentShaderId = addShader("Triangle Fragment Shader", "Assets/shaders/triangle/frag.spv");

			vertexLayoutId = addVerticeLayout("Triangle Vertice Layout");
			vertexMemberIds.positionId = addMemberToVerticeLayout(vertexLayoutId, "Position", VEC3);
			vertexMemberIds.colorId = addMemberToVerticeLayout(vertexLayoutId, "Color", VEC4);

			/* One uniform buffer descriptor read by the vertex shader (binding 0 = the only blueprint). Members follow the order of the MVP block in shader.vert. */
			dsLayoutId = addDescriptorSetLayout("DSLayout", UNIFORM_DESCRIPTOR, VK_SHADER_STAGE_VERTEX_BIT);
			mvpBlueprintId = addDescriptorBlueprint("DSLayout", "MVP");
			mvpMemberIds.projectionId = addMemberToDescriptorBlueprint(dsLayoutId, mvpBlueprintId, "Projection", MAT4);
			mvpMemberIds.viewId = addMemberToDescriptorBlueprint(dsLayoutId, mvpBlueprintId, "View", MAT4);
			mvpMemberIds.modelId = addMemberToDescriptorBlueprint(dsLayoutId, mvpBlueprintId, "Model", MAT4);

			/* The shader declares the descriptor it reads, so drawMeshIndexed can skip the draw until its first data was copied. The pipeline builds its pipeline layout from these declarations, so they come before the pipeline is added. */
			addDescriptorToShader(vertexShaderId, 0, dsLayoutId, mvpBlueprintId);
			graphicsPipelineId = addGraphicsPipelineLayout("Draw Triangle", vertexShaderId, fragmentShaderId, vertexLayoutId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, getPresentationRenderPass(), 0);
		}

		void defineResources()
		{
			meshId = addMesh("Triangle", vertexLayoutId, 3);
			Mesh& rMesh = getMesh(meshId); 
			rMesh.indices = { 
		 		0, 1, 2
			};
		
			/* Upload Mesh. The vertex data is written straight into the upload staging range, commitUpload queues the copy. */
			rMesh.queueUpload(DYNAMIC, GPU_LOCAL); 

			rMesh.setVerticeMember(vertexMemberIds.positionId, 0, glm::vec3(0.0, 0.5, 0.0));
			rMesh.setVerticeMember(vertexMemberIds.positionId, 1, glm::vec3(0.5, -0.5, 0.0));
			rMesh.setVerticeMember(vertexMemberIds.positionId, 2, glm::vec3(-0.5, -0.5, 0.0));

			rMesh.setVerticeMember(vertexMemberIds.colorId, 0, glm::vec4(1.0, 0.0, 0.0, 1.0)); 
			rMesh.setVerticeMember(vertexMemberIds.colorId, 1, glm::vec4(0.0, 1.0, 0.0, 1.0));
			rMesh.setVerticeMember(vertexMemberIds.colorId, 2, glm::vec4(0.0, 0.0, 1.0, 1.0));
		
			rMesh.commitUpload();

			/* STATIC sets have to be added before the static allocation, which happens right after define*. CPU_SHARED and not readOnly gives one entry per frame in flight, written through a CPU shadow. */
			descriptorSetId = addDescriptorSet(dsLayoutId, STATIC, CPU_SHARED);
		}

		void definePasses()
		{
			PassId passId = addPass("Triangle Pass", FRAME, GRAPHICS);
			uint32_t taskId = addTaskToPass(passId, "Draw Triangle", recordCMDs); 
		
			addDynamicWaitSemaphoreToTask(passId, taskId, getCurrentFrameImageIsAcquiredSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			addDynamicSignalSemaphoreToTask(passId, taskId, getCurrentSwapchainImageUseFinishedSemaphore);
			enablePass(passId); 
		}

		void updateResources()
		{
			counter++; 

			/* The descriptor memory exists only after define*, so the values can't be written in defineResources. The camera sits at z = 2 and looks at the origin, the model stays at the origin. */
			if(!descriptorsHaveSet)
			{
				DescriptorSet& rSet = getDescriptorSet(descriptorSetId);
				rSet.setDescriptorMemberValue(mvpBlueprintId, mvpMemberIds.viewId, glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
				rSet.setDescriptorMemberValue(mvpBlueprintId, mvpMemberIds.modelId, glm::mat4(1.0f));
				descriptorsHaveSet = true; 
			}

			/* The projection depends on the swapchain aspect ratio, so it is rebuilt when the extent changes (a resize recreates the swapchain). perspectiveRH_ZO gives the 0..1 depth range Vulkan uses. Vulkan's clip space has Y pointing down, unlike GLM's, so [1][1] is negated to keep +Y up on screen. */
			const VkExtent2D& rExtent = getSwapchain().extent;
			if(rExtent.width != lastProjectionExtent.width || rExtent.height != lastProjectionExtent.height)
			{
				glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(45.0f), static_cast<float>(rExtent.width) / static_cast<float>(rExtent.height), 0.1f, 100.0f);
				projection[1][1] *= -1.0f;
				DescriptorSet& rSet = getDescriptorSet(descriptorSetId);
				rSet.setDescriptorMemberValue(mvpBlueprintId, mvpMemberIds.projectionId, projection);
				lastProjectionExtent = rExtent;
			}

			if(counter == 7000)
			{
				Mesh& rMesh = getMesh(meshId);
				rMesh.setVerticeMember(vertexMemberIds.colorId, 0, glm::vec4(0.0, 0.5, 0.5, 1.0)); 			
				rMesh.setVerticeMember(vertexMemberIds.colorId, 1, glm::vec4(0.5, 0.0, 0.5, 1.0)); 			
				rMesh.setVerticeMember(vertexMemberIds.colorId, 2, glm::vec4(0.5, 0.5, 0.0, 1.0)); 			
			}
			
			if (counter > 14000)
			{
				counter = 0; 
				Mesh& rMesh = getMesh(meshId);
				rMesh.setVerticeMember(vertexMemberIds.colorId, 0, glm::vec4(1.0, 0.0, 0.0, 1.0)); 
				rMesh.setVerticeMember(vertexMemberIds.colorId, 1, glm::vec4(0.0, 1.0, 0.0, 1.0));
				rMesh.setVerticeMember(vertexMemberIds.colorId, 2, glm::vec4(0.0, 0.0, 1.0, 1.0));
			}
		}

		void recordCMDs(VkCommandBuffer& cmdBuffer_)
		{
			beginCMDsRecording(cmdBuffer_); 
			// Bind graphics pipeline  
			bindGraphicsPipeline(graphicsPipelineId, cmdBuffer_); 
			/* drawMeshIndexed looks up the declared descriptors in the set bound at the shader's set index, so the set has to be bound before the draw. */
			bindDescriptorSet(descriptorSetId, graphicsPipelineId, 0, cmdBuffer_);
			// Bind and draw vertex buffer 
			drawMeshIndexed(meshId, 1, cmdBuffer_); 
			endCMDsRecording(cmdBuffer_); 
		}
	}
}
