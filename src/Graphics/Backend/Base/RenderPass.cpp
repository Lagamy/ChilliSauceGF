// Renderpass decides how a fixed set of Attachments(images) are used across subpasses, including their layout transitions and synchronization. 

// Attachment is simply an image (texture, color buffer, depth buffer, etc.)

/*
Attachment layouts:

The GPU can store the same image data in different memory arrangements depending on how it’s used:
When you are rendering to it, the GPU may use a tiled layout optimized for writes.

When you are sampling it in a shader, it needs a read-optimized layout.

When you present it to the display engine, it needs a linear or present layout that the scanout hardware can read.

*/
#include "RenderPass.h" 
#include "Globals.h"

void RenderPass::addSubpass(SubPassDescriptionInfo subpassDescriptionInfo_, SubPassLayoutTransitionInfo subpassTransitionInfo_)
{
	this->subpassDependencies.emplace_back();
	this->subpassDescriptions.emplace_back();
	this->subpasses.emplace_back(this->subpassDescriptions.back(), this->subpassDependencies.back(), subpassDescriptionInfo_, subpassTransitionInfo_);
}


//void RenderPass::create() {
//	VkRenderPassCreateInfo createInfo = {};
//	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//
//	// --------------------Attachments-------------------------------- 
//	// Color attachment of Render Pass(Output to a frame buffer) 
//	VkAttachmentDescription colorAttachment = {};
//	colorAttachment.format = EngineGlobals::renderer.swapchain.imageFormat;				// Format to use for Attachment(image)
//	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;									// Number of sample to write for Multisampling
//	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;								// Describes what to do with attachment before rendering. Clear image before render pass starts. 
//	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;								// Describes what to do with attachment after rendering. Store result, so I can draw it to the Framebuffer later.
//	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// I dont have Depth/Stencil yet. 
//	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	
//	// Framebuffer data can be stored as image, but images can be given different data layouts 
//	// to give optimal use for certain operations. Image that shader can read - have a different data layout, than Image that gets presented to Screen 
//	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;							// Image data layout before render pass starts. In what format image comes as.
//	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;						// Change to that image data layout after render pass. Image data layout for presenting to the Surface 
//	// --------------------------------------------------------------
//
//	createInfo.attachmentCount = 1;
//	createInfo.pAttachments = &colorAttachment;											// Inputs/Outputs. color output to framebuffer. Corespond to out(layout = 0) in fragment shader. 
//
//	// -----------------------Subpasses-----------------------------
//	/* Information about a particular Subpass the Render Pass is using */
//
//	// Attachment reference uses an attachment index, that referce to index in the attachment list passed to renderPassCreateInfo 
//	// You could create different references to same attachment with different layouts. To satisfy each Subpass. 
//	VkAttachmentReference colorAttachmentReference = {};
//	colorAttachmentReference.attachment = 0; // Use first one 
//	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;				// Change to that image layout for Subpasses to write it.  
//	VkSubpassDescription subpass = {};
//	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;						// Pipeline type subpass is about to be bound to. 
//	subpass.colorAttachmentCount = 1;
//	subpass.pColorAttachments = &colorAttachmentReference;								// Provide attachment references list
//
//	// ------------------------------------------------------------
//	createInfo.subpassCount = 1;
//	createInfo.pSubpasses = &subpass;
//
//	// -----------------------------------------------------------------Layout Transitions---------------------------------------------------------------------------- 
//	/*
//	Need to determine when layout transitions occur using subpass dependencies.
//	(For example transition attachment layout from VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_ATTACHMENT_SHADER_READONLY, when Subpass 1 no longer uses it)
//	Tells "at that point -> you can start conversion, even tho Subpass 1 haven't finished rendering, as it wont touch this attachment anymore.
//	And before Subpass 2 tries to write or access fragment shader"
//	Bassically setting a window when conversion should happen.
//	*/
//	std::array<VkSubpassDependency, 2> subpassDependencies;
//
//	/* Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL(Initial Image layout->layout for Subpass 1) */
//	// Convert after VK_SUBPASS_EXTERNAL last read 
//	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;							// VK_SUBPASS_EXTERNAL = Anything that takes place outside of Renderpass. 
//	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			// Pipeline Stage after which we can convert in general. 
//	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;					// Last operation/s in this stage, after which we can convert. 
//	// Convert before Subpass 1, and VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT has to read and write.
//	subpassDependencies[0].dstSubpass = 0;												 // Id of our first Subpass.  
//	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Pipeline Stage in Subpass before which we need convert in general. 
//	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Operation/s in that stage, before which we must convert
//	subpassDependencies[0].dependencyFlags = 0; // Regions 
//
//	/* Conversion from VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR(Subpass 1 Image layout->layout for Presenting to Screen) */
//	// Convert after Subpass 1 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT last read and write.  
//	subpassDependencies[1].srcSubpass = 0;												// VK_SUBPASS_EXTERNAL = Anything that takes place outside of Renderpass. 
//	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;// Pipeline Stage after which we can convert in general. 
//	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Last operation/s in this stage after which we can convert.
//	// Convert before Pipeline is ended(Output), and VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT is sending an image to a Present Framebuffer.
//	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;												 // Id of our first Subpass.  
//	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline Stage before before which we need convert in general. 
//	subpassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT; // Operation/s in that stage, before which we must convert
//	subpassDependencies[1].dependencyFlags = 0; // Regions 
//	// ---------------------------------------------------------------------------------------------------------------------------------------------------------------- 
//
//	createInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
//	createInfo.pDependencies = subpassDependencies.data();	// Describes when changes between different subpasses need to occure. 
//	VkResult result = vkCreateRenderPass(EngineGlobals::renderer.mainDevice.logicalDevice, &createInfo, nullptr, &vkHandle);
//	if (result != VK_SUCCESS)
//	{
//		throw std::runtime_error("Failed to create RenderPass!");
//	}
//}


void RenderPass::create() {

	this->finalAttachments.reserve(this->colorAttachments.size() + this->depthStencilAttachments.size() + this->resolveAttachments.size());
	this->finalAttachments.insert(this->finalAttachments.begin(), this->colorAttachments.begin(), this->colorAttachments.end());
	this->finalAttachments.insert(this->finalAttachments.end(), this->depthStencilAttachments.begin(), this->depthStencilAttachments.end());
	this->finalAttachments.insert(this->finalAttachments.end(), this->resolveAttachments.begin(), this->resolveAttachments.end());

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	// --------------------Attachments-------------------------------- 
	// Color attachment of Render Pass(Output to a frame buffer) 

	// --------------------------------------------------------------

	createInfo.attachmentCount = this->finalAttachments.size();
	createInfo.pAttachments = this->finalAttachments.data();	// Inputs/Outputs. color output to framebuffer. Corespond to out(layout = 0) in fragment shader. 

	// -----------------------Subpasses-----------------------------
	/* Information about a particular Subpass the Render Pass is using */

	// Attachment reference uses an attachment index, that referce to index in the attachment list passed to renderPassCreateInfo 
	// You could create different references to same attachment with different layouts. To satisfy each Subpass. 

	// ------------------------------------------------------------
	createInfo.subpassCount = this->subpassDescriptions.size();
	createInfo.pSubpasses = this->subpassDescriptions.data();

	// -----------------------------------------------------------------Layout Transitions---------------------------------------------------------------------------- 
	subpasses[0].initFirstLayoutTrasition(); 
	for (size_t i = 1; i < subpasses.size(); i++)
	{
		this->subpasses[i].initLayoutTrasition(this->subpasses[i - 1].rLayoutTransition, i); // Initialize transition from previous to current
	}

	addFinalExternalDependency();
	// ---------------------------------------------------------------------------------------------------------------------------------------------------------------- 

	createInfo.dependencyCount = static_cast<uint32_t>(this->subpassDependencies.size());
	createInfo.pDependencies = this->subpassDependencies.data();	// Describes when changes between different subpasses need to occure. 
	VkResult result = vkCreateRenderPass(Demo::renderer.mainDevice.logicalDevice, &createInfo, nullptr, &vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create RenderPass!");
	}
}

void RenderPass::addFinalExternalDependency()
{
	if (subpasses.empty()) return;

	const SubPass& last = subpasses.back();

	VkSubpassDependency finalDependency = {};
	finalDependency.srcSubpass = subpasses.size() - 1;
	finalDependency.srcStageMask = last.rLayoutTransition.dstStageMask;   // whatever last subpass wrote
	finalDependency.srcAccessMask = last.rLayoutTransition.dstAccessMask;
	finalDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	finalDependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline Stage before before which we need convert in general. 
	finalDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT; // Operation/s in that stage, before which we must convert
	finalDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	this->subpassDependencies.emplace_back(finalDependency);
}

void RenderPass::addColorAttachment(GraphicsUtilities::ColorFormatsEnum format_, VkSampleCountFlagBits sampleCount_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->colorAttachments.emplace_back();
	VkAttachmentDescription& newColorAttachment = this->colorAttachments.back();
	newColorAttachment = {}; 
	newColorAttachment.format = static_cast<VkFormat>(format_);	// Format to use for Attachment(image)
	newColorAttachment.samples = sampleCount_;	// Number of sample to write for Multisampling
	newColorAttachment.loadOp = loadOp_;	// Describes what to do with attachment before rendering. Clear image before render pass starts. 
	newColorAttachment.storeOp = storeOp_;	// Describes what to do with attachment after rendering. Store result, so I can draw it to the Framebuffer later.
	newColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// Doesn't have Depth/Stencil yet. 
	newColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	newColorAttachment.initialLayout = initialLayout_;// The layout before renderpass begins
	newColorAttachment.finalLayout = finalLayout_; // The layout after renderpass ends
}

void RenderPass::addDepthStencilAttachment(GraphicsUtilities::DepthStencilFormatsEnum format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkAttachmentLoadOp stencilLoadOp_, VkAttachmentStoreOp stencilStoreOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->depthStencilAttachments.emplace_back();
	VkAttachmentDescription& newDepthStencilAttachment = this->colorAttachments.back();
	newDepthStencilAttachment = {};
	newDepthStencilAttachment.format = static_cast<VkFormat>(format_);	// Format to use for Attachment(image)
	newDepthStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	// Number of sample to write for Multisampling
	newDepthStencilAttachment.loadOp = loadOp_;	// Describes what to do with attachment before rendering. Clear image before render pass starts. 
	newDepthStencilAttachment.storeOp = storeOp_;	// Describes what to do with attachment after rendering. Store result, so I can draw it to the Framebuffer later.
	newDepthStencilAttachment.stencilLoadOp = stencilLoadOp_;
	newDepthStencilAttachment.stencilStoreOp = stencilStoreOp_;
	newDepthStencilAttachment.initialLayout = initialLayout_;// The layout before renderpass begins
	newDepthStencilAttachment.finalLayout = finalLayout_; // The layout after renderpass ends
}

void RenderPass::addDepthAttachment(GraphicsUtilities::DepthFormatsEnum format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->depthStencilAttachments.emplace_back();
	VkAttachmentDescription& newDepthAttachment = this->colorAttachments.back();
	newDepthAttachment = {};
	newDepthAttachment.format = static_cast<VkFormat>(format_);	// Format to use for Attachment(image)
	newDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	// Number of sample to write for Multisampling
	newDepthAttachment.loadOp = loadOp_;	// Describes what to do with attachment before rendering. Clear image before render pass starts. 
	newDepthAttachment.storeOp = storeOp_;	// Describes what to do with attachment after rendering. Store result, so I can draw it to the Framebuffer later.
	newDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// Doesn't have Depth/Stencil yet. 
	newDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	newDepthAttachment.initialLayout = initialLayout_;// The layout before renderpass begins
	newDepthAttachment.finalLayout = finalLayout_; // The layout after renderpass ends
}

void RenderPass::addStencilAttachment(VkAttachmentLoadOp stencilLoadOp_, VkAttachmentStoreOp stencilStoreOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->depthStencilAttachments.emplace_back();
	VkAttachmentDescription& newStencilAttachment = this->colorAttachments.back();
	newStencilAttachment = {};
	newStencilAttachment.format = VK_FORMAT_S8_UINT;	// Format to use for Attachment(image)
	newStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	// Number of sample to write for Multisampling
	newStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	newStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	newStencilAttachment.stencilLoadOp = stencilLoadOp_;				
	newStencilAttachment.stencilStoreOp = stencilStoreOp_;
	newStencilAttachment.initialLayout = initialLayout_; // The layout before renderpass begins
	newStencilAttachment.finalLayout = finalLayout_; // The layout after renderpass ends
}

void RenderPass::addResolveAttachment(GraphicsUtilities::ColorFormatsEnum format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->resolveAttachments.emplace_back();
	VkAttachmentDescription& newColorAttachment = this->colorAttachments.back();
	newColorAttachment = {};
	newColorAttachment.format = static_cast<VkFormat>(format_);	// Format to use for Attachment(image)
	newColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	// Number of sample to write for Multisampling
	newColorAttachment.loadOp = loadOp_;	// Describes what to do with attachment before rendering. Clear image before render pass starts. 
	newColorAttachment.storeOp = storeOp_;	// Describes what to do with attachment after rendering. Store result, so I can draw it to the Framebuffer later.
	newColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// Doesn't have Depth/Stencil yet. 
	newColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	newColorAttachment.initialLayout = initialLayout_;// The layout before renderpass begins
	newColorAttachment.finalLayout = finalLayout_; // The layout after renderpass ends
}

size_t RenderPass::getColorAttachmentIdFromFinal(size_t localId_)
{
	return localId_; // Color attachments is first vector to be added to finalAttachments 
}

size_t RenderPass::getDepthStencilAttachmentIdFromFinal(size_t localId_)
{
	return this->colorAttachments.size() + localId_;
}

size_t RenderPass::getResolveAttachmentIdFromFinal(size_t localId_)
{
	return this->colorAttachments.size() + this->depthStencilAttachments.size() + localId_;
}


void RenderPass::destroy() {
	vkDestroyRenderPass(Demo::renderer.mainDevice.logicalDevice, vkHandle, nullptr);
	vkHandle = VK_NULL_HANDLE;
}


VkRenderPass RenderPass::get() const
{
	return vkHandle;
}
