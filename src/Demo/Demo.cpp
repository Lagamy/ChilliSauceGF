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
			vertexVariableIds.positionId = addVariableToVerticeLayout(vertexLayoutId, "Position", VEC3);
			vertexVariableIds.colorId = addVariableToVerticeLayout(vertexLayoutId, "Color", VEC4);

			/* Descriptors read by the vertex shader. The descriptor layout slot index is the binding number, so the creation order has to match shader.vert: MVP = 0, Objects = 1, Counter = 2. Members follow the order of the blocks in shader.vert. */
			dsLayoutId = addDescriptorSetLayout("DSLayout", VK_SHADER_STAGE_VERTEX_BIT);

			vpDescriptorLayoutId = addBufferDescriptorLayout(dsLayoutId, "MVP", UNIFORM_DESCRIPTOR);
			vpMemberIds.projectionId = addMemberToDescriptorLayout(dsLayoutId, vpDescriptorLayoutId, "Projection", MAT4);
			vpMemberIds.viewId = addMemberToDescriptorLayout(dsLayoutId, vpDescriptorLayoutId, "View", MAT4);


			/* Same shape as the block in shader.vert: states[2] followed by model, the whole block repeated once per triangle (dynamic unit). */
			objectsDescriptorLayoutId = addBufferDynamicDescriptorLayout(dsLayoutId, "Objects", STORAGE_DESCRIPTOR, /*dynamicCount_*/ 2);
			objectsMemberIds.statesId = addStructMemberToDescriptorLayout(dsLayoutId, objectsDescriptorLayoutId, "states", {2});
			objectsMemberIds.colorId = addVariableToDescriptorLayout(dsLayoutId, objectsDescriptorLayoutId, objectsMemberIds.statesId, "color", VEC4);
			objectsMemberIds.rotationDirectionId = addVariableToDescriptorLayout(dsLayoutId, objectsDescriptorLayoutId, objectsMemberIds.statesId, "rotationDirection", INT32);
			objectsMemberIds.modelId = addMemberToDescriptorLayout(dsLayoutId, objectsDescriptorLayoutId, "model", MAT4);

			counterDescriptorLayoutId = addBufferDescriptorLayout(dsLayoutId, "Counter", UNIFORM_DESCRIPTOR);
			counterMemberId = addMemberToDescriptorLayout(dsLayoutId, counterDescriptorLayoutId, "Value", UINT32);

			/* The shader declares the descriptor it reads, so drawMeshIndexed can skip the draw until its first data was copied. The pipeline builds its pipeline layout from these declarations, so they come before the pipeline is added. */
			addDescriptorsToShader(vertexShaderId, 0, dsLayoutId, {vpDescriptorLayoutId, objectsDescriptorLayoutId, counterDescriptorLayoutId});
			graphicsPipelineId = addGraphicsPipelineLayout("Draw Triangle", vertexShaderId, fragmentShaderId, vertexLayoutId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, getPresentationRenderPass(), 0);
		}

		void defineResources()
		{
			triangle = Triangle(vertexLayoutId, vertexVariableIds);
			triangle.upload(DYNAMIC_ALLOCATOR, GPU_LOCAL);

			/* STATIC_ALLOCATOR sets have to be added before the static allocation, which happens right after define*. CPU_SHARED and not readOnly gives one entry per frame in flight, written through a CPU shadow. */
			descriptorSetId = addDescriptorSet(dsLayoutId, STATIC_ALLOCATOR, CPU_SHARED);
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
			rSet.setDescriptorMemberValue(counterDescriptorLayoutId, counterMemberId, counter);   // per frame
			
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

		void recordCMDs(VkCommandBuffer& cmdBuffer_)
		{
			beginCMDsRecording(cmdBuffer_); 
			// Bind graphics pipeline  
			bindGraphicsPipeline(graphicsPipelineId, cmdBuffer_); 
			
			/*
				Both triangles use the same set at set index 0 (the pipeline layout has one set). What differs is the dynamic unit: bindDescriptorSet takes one unit id per dynamic descriptor
				(only Objects is dynamic), which picks the block (states + model) of that triangle. drawMeshIndexed looks up the declared descriptors in the set bound at the shader's set index, so the set has to be bound before each draw.
			*/
			std::array<uint32_t, 1> firstTriangleUnitIds = {0};
			bindDescriptorSet(descriptorSetId, graphicsPipelineId, 0, cmdBuffer_, firstTriangleUnitIds);
			drawMeshIndexed(triangle.meshId, 1, cmdBuffer_);

			std::array<uint32_t, 1> secondTriangleUnitIds = {1};
			bindDescriptorSet(descriptorSetId, graphicsPipelineId, 0, cmdBuffer_, secondTriangleUnitIds);
			drawMeshIndexed(triangle.meshId, 1, cmdBuffer_);
			
			endCMDsRecording(cmdBuffer_); 
		}
	}
}
