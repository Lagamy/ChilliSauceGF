// Renderpass decides how a fixed set of Attachments(images) are used across subpasses, including their layout transitions and synchronization. 

// Attachment is simply an image (texture, color buffer, depth buffer, etc.)

/*
Attachment layouts:

The GPU can store the same image data in different memory arrangements depending on how it�s used:
When you are rendering to it, the GPU may use a tiled layout optimized for writes.

When you are sampling it in a shader, it needs a read-optimized layout.

When you present it to the display engine, it needs a linear or present layout that the scanout hardware can read.

*/
#include "RenderPass.h" 
#include "Api.h"

namespace Graphics
{
void RenderPass::addSubpass(SubPassDescriptionInfo subpassDescriptionInfo_, SubPassLayoutTransitionInfo subpassTransitionInfo_)
{
	this->subpassDependencies.emplace_back();
	this->subpassDescriptions.emplace_back();
	this->subpasses.emplace_back(this->subpassDescriptions.back(), this->subpassDependencies.back(), subpassDescriptionInfo_, subpassTransitionInfo_);
}



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
	VkResult result = vkCreateRenderPass(getMainDevice().logicalDevice, &createInfo, nullptr, &vkHandle);
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

void RenderPass::addColorAttachment(VkFormat format_, VkSampleCountFlagBits sampleCount_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->colorAttachments.emplace_back();
	VkAttachmentDescription& newColorAttachment = this->colorAttachments.back();
	newColorAttachment = {}; 
	newColorAttachment.format = format_;	// Format to use for Attachment(image)
	newColorAttachment.samples = sampleCount_;	// Number of sample to write for Multisampling
	newColorAttachment.loadOp = loadOp_;	// Describes what to do with attachment before rendering. Clear image before render pass starts. 
	newColorAttachment.storeOp = storeOp_;	// Describes what to do with attachment after rendering. Store result, so I can draw it to the Framebuffer later.
	newColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// Doesn't have Depth/Stencil yet. 
	newColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	newColorAttachment.initialLayout = initialLayout_;// The layout before renderpass begins
	newColorAttachment.finalLayout = finalLayout_; // The layout after renderpass ends
}

void RenderPass::addDepthStencilAttachment(VkFormat format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkAttachmentLoadOp stencilLoadOp_, VkAttachmentStoreOp stencilStoreOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->depthStencilAttachments.emplace_back();
	VkAttachmentDescription& newDepthStencilAttachment = this->depthStencilAttachments.back();
	newDepthStencilAttachment = {};
	newDepthStencilAttachment.format = format_;	// Format to use for Attachment(image)
	newDepthStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	// Number of sample to write for Multisampling
	newDepthStencilAttachment.loadOp = loadOp_;	// Describes what to do with attachment before rendering. Clear image before render pass starts. 
	newDepthStencilAttachment.storeOp = storeOp_;	// Describes what to do with attachment after rendering. Store result, so I can draw it to the Framebuffer later.
	newDepthStencilAttachment.stencilLoadOp = stencilLoadOp_;
	newDepthStencilAttachment.stencilStoreOp = stencilStoreOp_;
	newDepthStencilAttachment.initialLayout = initialLayout_;// The layout before renderpass begins
	newDepthStencilAttachment.finalLayout = finalLayout_; // The layout after renderpass ends
}

void RenderPass::addDepthAttachment(VkFormat format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->depthStencilAttachments.emplace_back();
	VkAttachmentDescription& newDepthAttachment = this->depthStencilAttachments.back();
	newDepthAttachment = {};
	newDepthAttachment.format = format_;	// Format to use for Attachment(image)
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
	VkAttachmentDescription& newStencilAttachment = this->depthStencilAttachments.back();
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

void RenderPass::addResolveAttachment(VkFormat format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_)
{
	this->resolveAttachments.emplace_back();
	VkAttachmentDescription& newColorAttachment = this->resolveAttachments.back();
	newColorAttachment = {};
	newColorAttachment.format = format_;	// Format to use for Attachment(image)
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
	return this->depthStencilAttachments.size() + localId_;
}

size_t RenderPass::getResolveAttachmentIdFromFinal(size_t localId_)
{
	return this->resolveAttachments.size() + this->depthStencilAttachments.size() + localId_;
}


void RenderPass::destroy() {
	vkDestroyRenderPass(getMainDevice().logicalDevice, vkHandle, nullptr);
	vkHandle = VK_NULL_HANDLE;
}


VkRenderPass RenderPass::get() const
{
	return vkHandle;
}
}
