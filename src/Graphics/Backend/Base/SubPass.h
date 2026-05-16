#pragma once 
#include <vulkan/vulkan.h>
#include <vector>
#include <span>

enum class DepthStencilAccessTypeEnum {
	DEPTH_STENCIL_OPTIMAL = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
	DEPTH_READ_ONLY = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
	DEPTH_WRITE_STENCIL_READ_ONLY = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
};

struct RenderPass; 

struct SubPassLayoutTransitionInfo {
	VkPipelineStageFlags stageMaskFlag;
	VkAccessFlags accessMaskFlag;
	VkDependencyFlags dependencyFlags;
};

struct SubPassDescriptionInfo {
	RenderPass* pRenderPass;
	VkPipelineBindPoint pipelineBindPoint;
	std::vector<uint32_t> colorAttachmentsToUseIds;
	const uint32_t depthStencilAttachmentToUseId; 
	DepthStencilAccessTypeEnum depthStencilAccessType;
	std::vector<uint32_t> resolveAttachmentsToUseIds;
	VkAttachmentReference* pInputAttachmentsRefs;
	uint32_t inputAttachmentsRefcount;
};

struct SubPass {
	VkSubpassDescription& rDescription;
	VkSubpassDependency& rLayoutTransition; 
	std::vector<VkAttachmentReference> colorAttachmentsRefs;
	// Only one depth/stencil attachment can be active per subpass.
	VkAttachmentReference depthStencilAttachmentRef;
	std::vector<VkAttachmentReference> resolveAttachmentsRefs;

	SubPass(VkSubpassDescription& rDescription_, VkSubpassDependency& rLayoutTransition_, SubPassDescriptionInfo subpassDescriptionInfo_, SubPassLayoutTransitionInfo subpassTransitionInfo_);
	void initLayoutTrasition(VkSubpassDependency& rPreviousSubpassDependancy_, size_t id_);
	void initFirstLayoutTrasition();

};
