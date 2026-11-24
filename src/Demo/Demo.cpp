#include "Demo.h"
#include "Api.h"
#include "DescriptorSet.h"
#include "Utilities.h"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
	namespace Demo
	{		
		void setGPUSceneToDemo()
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
			PoolId vertexUpdateId = beginVertexLayoutUpdate(vertexLayoutId);
			vertexVariableIds.positionId = addVariableToVerticeLayout("Position", vertexUpdateId, 0, VEC3);
			vertexVariableIds.colorId = addVariableToVerticeLayout("Color", vertexUpdateId, 1, VEC4);
			commitVertexLayoutUpdate(vertexUpdateId);

			/* Descriptors read by the vertex shader. The set number (0) and the binding numbers (MVP = 0, Objects = 1, Counter = 2) have to match shader.vert. The order positions of the members follow the order of the blocks in shader.vert. Nothing exists on the GPU yet, so the commits only write the layouts. */
			dsLayoutId = addDescriptorSetLayout("DSLayout", 0, VK_SHADER_STAGE_VERTEX_BIT);
			PoolId setLayoutUpdateId = beginSetLayoutUpdate(dsLayoutId);

			vpDescriptorLayoutId = addBufferDescriptorLayout("MVP", setLayoutUpdateId, 0, UNIFORM_DESCRIPTOR);
			PoolId vpUpdateId = beginDescriptorLayoutUpdate(dsLayoutId, vpDescriptorLayoutId);
			vpMemberIds.projectionId = addMemberToDescriptorLayout("Projection", vpUpdateId, 0, MAT4);
			vpMemberIds.viewId = addMemberToDescriptorLayout("View", vpUpdateId, 1, MAT4);
			commitDescriptorLayoutUpdate(vpUpdateId);

			/* Same shape as the block in shader.vert: states[2] followed by model, the whole block repeated once per triangle (dynamic unit). */
			objectsDescriptorLayoutId = addDynamicDescriptorLayout("Objects", setLayoutUpdateId, 1, STORAGE_DESCRIPTOR, /*dynamicCount_*/ 2);
			PoolId objectsUpdateId = beginDescriptorLayoutUpdate(dsLayoutId, objectsDescriptorLayoutId);
			objectsMemberIds.statesId = addStructMemberToDescriptorLayout("states", objectsUpdateId, 0, {2});
			objectsMemberIds.colorId = addVariableToStructInDescriptorLayout("color", objectsUpdateId, objectsMemberIds.statesId, 0, VEC4);
			objectsMemberIds.rotationDirectionId = addVariableToStructInDescriptorLayout("rotationDirection", objectsUpdateId, objectsMemberIds.statesId, 1, INT32);
			objectsMemberIds.modelId = addMemberToDescriptorLayout("model", objectsUpdateId, 1, MAT4);
			commitDescriptorLayoutUpdate(objectsUpdateId);
			commitSetLayoutUpdate(setLayoutUpdateId);

			/* The shader declares the descriptor it reads, so drawMeshIndexed can skip the draw until its first data was copied. The pipeline builds its pipeline layout from these declarations, so they come before the pipeline is added. */
			addDescriptorsToShader(vertexShaderId, dsLayoutId, {vpDescriptorLayoutId, objectsDescriptorLayoutId});
			graphicsPipelineId = addGraphicsPipelineLayout("Draw Triangle", vertexShaderId, fragmentShaderId, vertexLayoutId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, getPresentationRenderPass(), 0);
			
			pcLayoutId = addPushConstantLayout("Counter"); 
			PoolId pcLayoutUpdateId = beginPushConstantLayoutUpdate(pcLayoutId); 
			addMemberToPushConstantLayout("counter", pcLayoutUpdateId, 0, UINT32); 
			addMemberToPushConstantLayout("posOffsetY", pcLayoutUpdateId, 1, FLOAT);
			commitPushConstantLayoutUpdate(pcLayoutUpdateId);

			addPushConstantLayoutToGraphicsPipeline(graphicsPipelineId, pcLayoutId, VK_SHADER_STAGE_VERTEX_BIT); 
		}

		void defineResources()
		{
			triangle = Triangle(vertexLayoutId, vertexVariableIds);
			triangle.upload(DYNAMIC_ALLOCATOR, GPU_LOCAL);

			/* STATIC_ALLOCATOR sets have to be added before the static allocation, which happens right after define*. CPU_SHARED and not readOnly gives one entry per frame in flight, written through a CPU shadow. */
			DescriptorSetCreateParams setParams = {};
			setParams.layoutId = dsLayoutId;
			setParams.allocatorType = STATIC_ALLOCATOR;
			setParams.memoryVisability = CPU_SHARED;
			setParams.residency.absenceSecondsCount = UninitializedId;
			descriptorSetId = addDescriptorSet(setParams);
		}

		void definePasses()
		{
			PassId passId = addPass("Triangle Pass", FRAME, GRAPHICS);

			/*
				Both triangles use the same set at set index 0 (the pipeline layout has one set). What differs is the dynamic unit: the bind takes one unit id per dynamic descriptor
				(only Objects is dynamic), which picks the block (states + model) of that triangle. drawMeshIndexed looks up the declared descriptors in the set bound at the shader's set index, so the set is bound before each draw.
			*/
			drawTriangleCommandListId = addCommandList("Draw Triangle Commands");
			CommandList& rCommandList = getCommandList(drawTriangleCommandListId);
			rCommandList.addBeginPresentationRenderPass();
			rCommandList.addBindGraphicsPipeline(graphicsPipelineId);
			rCommandList.addSetPushConstantMemberValue(pcLayoutId, 0, &counter); 
			std::array<uint32_t, 1> firstTriangleUnitIds = {0};
			rCommandList.addBindDescriptorSet(descriptorSetId, firstTriangleUnitIds);
			rCommandList.addSetPushConstantMemberValue(pcLayoutId, 1, 0.4f); 
			rCommandList.addDrawMeshIndexed(triangle.meshId);
			std::array<uint32_t, 1> secondTriangleUnitIds = {1};
			rCommandList.addBindDescriptorSet(descriptorSetId, secondTriangleUnitIds);
			rCommandList.addSetPushConstantMemberValue(pcLayoutId, 1, -0.4f); 
			rCommandList.addDrawMeshIndexed(triangle.meshId);
			rCommandList.addEndRenderPass();
			
			uint32_t taskId = addTaskToPass("Draw Triangle", passId, drawTriangleCommandListId);
			addDynamicWaitSemaphoreToTask(passId, taskId, getCurrentFrameImageIsAcquiredSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			addDynamicSignalSemaphoreToTask(passId, taskId, getCurrentSwapchainImageUseFinishedSemaphore);
			enablePass(passId); 
		}

		void updateResources()
		{

			DescriptorSet& rSet = getDescriptorSet(descriptorSetId);
			/* The descriptor memory exists only after define*, so the values can't be written in defineResources. The camera sits at z = 2 and looks at the origin, the models (per triangle) stay at the origin. */
			if(!descriptorsHaveSet)
			{
				rSet.setDescriptorMemberValue(vpDescriptorLayoutId, vpMemberIds.viewId, glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
				/* GLM has no setPosition: the translation of a model matrix is its last column, so assigning it replaces the position (in world units, the scale doesn't touch it) instead of stacking on the previous one like glm::translate. */
				glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3( 0.5, 0.5, 0.5));

				modelMatrix[3] = glm::vec4(-0.5, 0, 0, 1.0);
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.modelId, modelMatrix, 0);

				modelMatrix[3] = glm::vec4(0.5, 0, 0, 1.0);
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.modelId, modelMatrix, 1);

				// setDynamicDescriptorMemberValue(descriptor layout, variable, value, dynamicUnitId = triangle, { struct ids = state })
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.colorId, glm::vec4(1, 0, 0, 1),   0, {0}); // triangle 0, state 0
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.rotationDirectionId, 1, 0, {0});

				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.colorId, glm::vec4(0, 1, 0, 1),   0, {1}); // triangle 0, state 1
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.rotationDirectionId, -1, 0, {1});

				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.colorId, glm::vec4(0, 0, 1, 1),   1, {0}); // triangle 1, state 0
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.rotationDirectionId, -1, 1, {0});
				
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.colorId, glm::vec4(1, 1, 0, 1),   1, {1}); // triangle 1, state 1
				rSet.setDynamicDescriptorMemberValue(objectsDescriptorLayoutId, objectsMemberIds.rotationDirectionId, 1, 1, {1});

				descriptorsHaveSet = true;
			}

			counter++;
			if(counter > 14000) counter = 0;                        // reset on the CPU, as now
			
			/* The projection depends on the swapchain aspect ratio, so it is rebuilt when the extent changes (a resize recreates the swapchain). perspectiveRH_ZO gives the 0..1 depth range Vulkan uses. Vulkan's clip space has Y pointing down, unlike GLM's, so [1][1] is negated to keep +Y up on screen. */
			const VkExtent2D& rExtent = getSwapchain().extent;
			if(rExtent.width != lastProjectionExtent.width || rExtent.height != lastProjectionExtent.height)
			{
				glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(45.0f), static_cast<float>(rExtent.width) / static_cast<float>(rExtent.height), 0.1f, 100.0f);
				projection[1][1] *= -1.0f;
				DescriptorSet& rSet = getDescriptorSet(descriptorSetId);
				rSet.setDescriptorMemberValue(vpDescriptorLayoutId, vpMemberIds.projectionId, projection);
				lastProjectionExtent = rExtent;
			}
		}
	}
}
