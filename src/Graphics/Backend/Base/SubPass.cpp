#include "SubPass.h"
#include "RenderPass.h"


SubPass::SubPass(VkSubpassDescription& rDescription_, VkSubpassDependency& rLayoutTransition_, SubPassDescriptionInfo subpassDescriptionInfo_, SubPassLayoutTransitionInfo subpassTransitionInfo_)
	: rDescription(rDescription_), rLayoutTransition(rLayoutTransition_)
{
	// Description initialization:
	this->colorAttachmentsRefs.resize(subpassDescriptionInfo_.colorAttachmentsToUseIds.size());
	this->resolveAttachmentsRefs.resize(subpassDescriptionInfo_.resolveAttachmentsToUseIds.size());

	for (size_t i = 0; i < subpassDescriptionInfo_.colorAttachmentsToUseIds.size(); i++)
	{
		this->colorAttachmentsRefs[i] = {};
		this->colorAttachmentsRefs[i].attachment = subpassDescriptionInfo_.rRenderPass.getColorAttachmentIdFromFinal(subpassDescriptionInfo_.colorAttachmentsToUseIds[i]);
		this->colorAttachmentsRefs[i].layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	}

	this->depthStencilAttachmentRef = {};
	this->depthStencilAttachmentRef.attachment = subpassDescriptionInfo_.rRenderPass.getDepthStencilAttachmentIdFromFinal(subpassDescriptionInfo_.depthStencilAttachmentToUseId);
	this->depthStencilAttachmentRef.layout = static_cast<VkImageLayout>(subpassDescriptionInfo_.depthStencilAccessType);

	for (size_t i = 0; i < subpassDescriptionInfo_.resolveAttachmentsToUseIds.size(); i++)
	{
		this->resolveAttachmentsRefs[i] = {};
		this->resolveAttachmentsRefs[i].attachment = subpassDescriptionInfo_.rRenderPass.getColorAttachmentIdFromFinal(subpassDescriptionInfo_.resolveAttachmentsToUseIds[i]);
		this->resolveAttachmentsRefs[i].layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	}

	this->rDescription = {};
	this->rDescription.pipelineBindPoint = subpassDescriptionInfo_.pipelineBindPoint;	// Pipeline type subpass is about to be bound to. 
	this->rDescription.pColorAttachments = this->colorAttachmentsRefs.data();	// Provide attachment references list
	this->rDescription.colorAttachmentCount = this->colorAttachmentsRefs.size();
	this->rDescription.pDepthStencilAttachment = &this->depthStencilAttachmentRef;  // Vulkan does not support writing to multiple depth/stencil attachments simultaneously. So only 1 depth/stencil allowed
	this->rDescription.pResolveAttachments = this->resolveAttachmentsRefs.data();  // Resolve attachments are strictly 1:1 paired with color attachments
	this->rDescription.pInputAttachments = subpassDescriptionInfo_.pInputAttachmentsRefs;
	this->rDescription.inputAttachmentCount = subpassDescriptionInfo_.inputAttachmentsRefcount;


	// Layout Transition initialization:
	this->rLayoutTransition = {};
	this->rLayoutTransition.dependencyFlags = subpassTransitionInfo_.dependencyFlags;
	this->rLayoutTransition.dstStageMask = subpassTransitionInfo_.stageMaskFlag;
	this->rLayoutTransition.dstAccessMask = subpassTransitionInfo_.accessMaskFlag;
}

void SubPass::initFirstLayoutTrasition()
{
	this->rLayoutTransition.dstSubpass = 0;
	this->rLayoutTransition.srcSubpass = VK_SUBPASS_EXTERNAL;							// VK_SUBPASS_EXTERNAL = Anything that takes place outside of Renderpass. 
	this->rLayoutTransition.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			// Pipeline Stage after which we can convert in general. 
	this->rLayoutTransition.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;					// Last operation/s in this stage, after which we can convert
}

void SubPass::initLayoutTrasition(VkSubpassDependency& rPreviousSubpassDependancy_, size_t id_)
{
	this->rLayoutTransition.dstSubpass = id_;
	this->rLayoutTransition.srcSubpass = id_ - 1;
	this->rLayoutTransition.srcStageMask = rPreviousSubpassDependancy_.dstStageMask;
	this->rLayoutTransition.srcAccessMask = rPreviousSubpassDependancy_.dstAccessMask;
}
