// Framebuffer - connection between renderpass and attachments. It is an interface, that allows us to clear/draw to this attachments. 
// It can draw only to 1 set of attachments(color, depth, resolve, stencil of same subpass) at the time. Specified in renderpass.  
// Flow goes as follows: Graphics Pipeline -> Renderpass -> Framebuffer -> Attachment(image) -> Screen(via swapchain)

// Binds a render pass instance to a specific set of image attachments.
// Attachments in Framebuffer should correspond to attachments in Renderpass 1 to 1 

/*
VkFramebuffer
 |- color attachment -> VkImageView (swapchain image)
 |- depth attachment -> VkImageView (depth image)
 |- resolve attachment -> VkImageView (for MSAA)

RenderPass is needed for that reason:

RenderPass (defines *how* attachments are used)
   |
   v
Framebuffer (binds *actual* images to those attachments)

Basically what imageViews are bound together for this rendepass 
*/


#pragma once 

#include <vector>
#include <vulkan/vulkan.h>
#include "RenderPass.h"

struct Framebuffer {
    VkFramebuffer vkHandle = VK_NULL_HANDLE;
    std::vector<VkImageView> attachments; 
    void create(uint32_t width_, uint32_t height_, uint32_t layers_, RenderPass& rRenderpass_);
    void destroy();

    VkFramebuffer get() const;
    ~Framebuffer(); 
};
