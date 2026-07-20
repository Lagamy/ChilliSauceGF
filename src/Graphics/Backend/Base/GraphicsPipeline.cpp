#include "GraphicsPipeline.h"
#include "Api.h"
#include "Layout.h"
#include "RenderPass.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void GraphicsPipeline::create()
{
	this->pipelineLayout.create(); // Deal with it when i get to implementing DS support 
	// Graphics Pipeline requires an array of shader stages creates
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages; 

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;	// Shader Stage name 
	shaderStageCreateInfo.module = getShader(this->vertexShaderId).get(); // Shader module to be used by stage  
	shaderStageCreateInfo.pName = "main"; // Name of function in GLSL which would be used as entry point. 
	shaderStages.emplace_back(shaderStageCreateInfo); 

	shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;	// Shader Stage name 
	shaderStageCreateInfo.module = getShader(this->fragmentShaderId).get(); // Shader module to be used by stage  
	shaderStageCreateInfo.pName = "main"; // Name of function in GLSL which would be used as entry point. 
	shaderStages.emplace_back(shaderStageCreateInfo); 	

	/* Vertex Input */
	ReflectionLayout& rVerticeLayout = getVerticeLayout(this->verticeLayoutId); 
	// Describes how the data for a single vertex(position, color, texture coords, normals, etc) is laid out. 
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0; // Can bind multiple streams of data, this defines which one 
	//bindingDescription.stride =  Engine::Project::graphicsObjects.meshTypes.get(this->meshTypeId).layout.back().element.lastByteId + 1; // Size of a single vertex object with all info. Used for distinguishing between each vertex.   
	bindingDescription.stride = rVerticeLayout.size; // Size of 1 vertex in GPU, so even if we use here only few attributes from uploaded vertice - Vulkan knows how to get to the next one.
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	// Describes how to move between data after each vertex.
	// VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex 
	// VK_VERTEX_INPUT_RATE_INSTANCE	: Move on to the same vertex for the next instance(When instancing) 
	// Vertex Attributes and how data for each is defined within vertex. 

	// How attribute is defined within Vertex 
	std::vector<VkVertexInputAttributeDescription> attributesDescriptions;
	attributesDescriptions.resize(rVerticeLayout.memberBlueprints.size());
	
	for(uint32_t i = 0; i < attributesDescriptions.size(); i++)
	{
		MemberBlueprint& rMemberBlueprint = rVerticeLayout.memberBlueprints.get({i, 0}); 
		attributesDescriptions[i].offset = rMemberBlueprint.firstByteId;
		attributesDescriptions[i].binding = 0;
		attributesDescriptions[i].location = i;
		attributesDescriptions[i].format =  typeToVkFormat[rMemberBlueprint.dataType]; 
	}

	// Feed pipeline create info
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription; // List of Vertex Binding Descriptions(data spacing, stride info)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = attributesDescriptions.size();  
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributesDescriptions.data(); // List of Vertex Attributes Descriptions(data format, and where to bind to, and where from)


	/* Input Assembly. Assembles verticies into primitive(dot, line or triangle)*/
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = this->primitiveType; // Primitive to assemble vertices into
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE; // Allow overriding of "strip" topology, to start new primitives. 


	/* Viewport and Scissor(Can be usefull for Splitscreen multiplayer)*/
	// Create a viewport
	// VkViewport viewport = {};
	// viewport.x = 0.0f;
	// viewport.y = 0.0f;
	// viewport.width = static_cast<float>(getSwapchain().extent.width);
	// viewport.height = static_cast<float>(getSwapchain().extent.height);
	// viewport.minDepth = 0.0f; // min framebuffer depth
	// viewport.maxDepth = 1.0f; // max framebuffer depth

	// // Create a scissor
	// VkRect2D scissor = {};
	// scissor.offset = { 0, 0 }; // Offset to use region from
	// scissor.extent = getSwapchain().extent; // Extent to describe to which region to capture/use 

	// // Viewport State creation info
	// VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
	// viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	// viewportCreateInfo.viewportCount = 1;
	// viewportCreateInfo.pViewports = &viewport;
	// viewportCreateInfo.scissorCount = 1;
	// viewportCreateInfo.pScissors = &scissor;

	/* Dynamic States(A way to tell Pipeline you want something to not be backed into it. Letting you change values after Pipeline creation)*/
	// Dynamic States to enable
	std::array<VkDynamicState, 2> dynamicStates = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	/* Rasterizer(Converts triangles to a fragments on screen)*/
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE; // Cutting things that are too Close or too Far 
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // If you dont want to convert to fragments and just get/store data. Only suitable for Pipeline without FrameBuffer. 
	rasterizerCreateInfo.polygonMode = this->polygonMode; // How to Rasterize Primitives. How primitives are filled during rasterization(Select line for wireframe)
	rasterizerCreateInfo.lineWidth = 1.0f; // How thick lines should be when draw(Other than 1.0 - needs GPU extension)
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // Which face of a triangle to cull 
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // The side from which triangles are connected Clockwise -> Front side
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE; // Whether to add depth bias for fragments (good for stopping "shadow acne" in shadow mapping) 
	// To enable it - see additional params inside of Rasterizer create info for it.


	/* MultiSampling(MipMapping) Disabled  */
	// Use image resolve attachment if enabled(to generate mipmaps into). 
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE; // Disabled
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to use per fragment


	/* Blending */
	// Blending decides how to blend new color being written to a fragment, with the old value 
	// Blend Attachment State(How blending is handled) 
	VkPipelineColorBlendAttachmentState colorState = {};
	colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;// To what color channels - apply blending
	colorState.blendEnable = VK_TRUE;// Enable blending
	// src = new color, dst = old color
	// Blending uses equation: (srcColorBlendFactor * new Color) colorBlendOp (dstColorBlendFactor * old Color)
	colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Literally alpha value of src color
	colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorState.alphaBlendOp = VK_BLEND_OP_ADD; // Addative blending
	// Summarized: (VK_BLEND_FACTOR_SRC_ALPHA * new Color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old Color)
	// Alpha Blending:
	colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // * 1
	colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // * 0. So we get rid of it. 
	colorState.alphaBlendOp = VK_BLEND_OP_ADD;
	// Summarized: (1 * new alpha) + (0 * old alpha)

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE; // Alternative to use math calculations is to use logical operations(XOR, AND, CLEAR, COPY..etc)
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorState;


	/* Depth Stencil Testing*/
	// TODO: Set up depth stencil testing 

	/* Create Graphics Pipeline */
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2; // Vertex and Fragment shaders
	graphicsPipelineCreateInfo.pStages = shaderStages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportState;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo; 
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.layout = pipelineLayout.get();
	graphicsPipelineCreateInfo.renderPass = this->pRenderPass->get(); // Will be used by that RenderPass
	graphicsPipelineCreateInfo.subpass = this->subpassId; // Id of Subpass. 0 = First Subpass will use this Pipeline. (If other Subpass would also need it(rare) -> create a copy of this Pipeline with different Subpass)
	// Pipeline Derivatives 
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Base this pipeline from another pipeline(You get to only edit parts that differ from main Pipeline. Saves memory)
	graphicsPipelineCreateInfo.basePipelineIndex = -1; // Or Index of this Pipline other can derive from(in case multiple same pipelines at once) 

	if (pipelineLayout.get() == VK_NULL_HANDLE) {
		throw std::runtime_error("PipelineLayout was not created before creating pipeline!");
	}

	if (this->pRenderPass->get() == VK_NULL_HANDLE) {
		throw std::runtime_error("RenderPass was not created before creating pipeline!");
	}

	// You can create multiple different Pipelines at once from a list of them.
	VkResult result = vkCreateGraphicsPipelines(getMainDevice().logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &vkHandle); // PipelineCache - store info about that Pipeline for reuse(So it create copies of this pipeline later on really fast) 
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}
}

//void GraphicsPipeline::createPipelineLayout( VkPushConstantRange* _pushConstRangeList, uint32_t _pushConstRangeCount, std::string _context) 
//{
//	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
//	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//	layoutCreateInfo.setLayoutCount = 2;
//	layoutCreateInfo.pSetLayouts = desSetLayouts.data();
//	layoutCreateInfo.pushConstantRangeCount = _pushConstRangeCount;
//	layoutCreateInfo.pPushConstantRanges = _pushConstRangeList;
//
//	VkResult result = vkCreatePipelineLayout(AppGlobals::mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &pipelineLayout.get());
//	if (result != VK_SUCCESS)
//	{
//		throw std::runtime_error(std::format("Failed to create Loyaut for Graphics Pipeline for {}!", _context));
//	}
//}

void GraphicsPipeline::destroy() 
{
	pipelineLayout.destroy(); 
	vkDestroyPipeline(getMainDevice().logicalDevice, vkHandle, nullptr);
    vkHandle = VK_NULL_HANDLE; 
}

VkPipeline GraphicsPipeline::get() const
{
	return vkHandle;
}
}
