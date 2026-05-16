// Describes what happens to set of attachments during rendering.
// The framebuffer attachments (color, depth, etc.) And how they're used over one or more Subpasses.

// One Render pass can be reused with many framebuffers, many command buffers, and across many frames.
#pragma once
#include "Utilities.h"
#include "SubPass.h"
#include <vulkan/vulkan.h>
#include <vector>


struct RenderPass {
private:
	VkRenderPass vkHandle = VK_NULL_HANDLE;

	std::vector<VkAttachmentDescription> finalAttachments;

	std::vector<VkSubpassDescription> subpassDescriptions;
	std::vector<VkSubpassDependency> subpassDependencies;

	/* Attachments need to be fully filled BEFORE SubPasses are created */
	std::vector<VkAttachmentDescription> colorAttachments;
	std::vector<VkAttachmentDescription> depthStencilAttachments;
	std::vector<VkAttachmentDescription> resolveAttachments;

	std::vector<SubPass> subpasses;
public:
	void addSubpass(SubPassDescriptionInfo subpassDescriptionInfo_, SubPassLayoutTransitionInfo subpassTransitionInfo_); // returns SubPass reference, to init both description, and layout transition. 
	void create();
	void destroy();

	void addFinalExternalDependency();

	// In UI editing
	void addColorAttachment(GraphicsUtilities::ColorFormatsEnum format_, VkSampleCountFlagBits sampleCount_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_);
	void addDepthStencilAttachment(GraphicsUtilities::DepthStencilFormatsEnum format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkAttachmentLoadOp stencilLoadOp_, VkAttachmentStoreOp stencilStoreOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_);
	void addDepthAttachment(GraphicsUtilities::DepthFormatsEnum format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_);
	void addStencilAttachment(VkAttachmentLoadOp stencilLoadOp_, VkAttachmentStoreOp stencilStoreOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_);
	void addResolveAttachment(GraphicsUtilities::ColorFormatsEnum format_, VkAttachmentLoadOp loadOp_, VkAttachmentStoreOp storeOp_, VkImageLayout initialLayout_, VkImageLayout finalLayout_);

	size_t getColorAttachmentIdFromFinal(size_t localId_);
	size_t getDepthStencilAttachmentIdFromFinal(size_t localId_);
	size_t getResolveAttachmentIdFromFinal(size_t localId_);

	// Load from a saved binarie
	void loadFromBinnary(); 
	VkRenderPass get() const;
};
