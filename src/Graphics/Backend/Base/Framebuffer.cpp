#include "Framebuffer.h"
#include "Api.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace Graphics
{
void Framebuffer::create(uint32_t width_, uint32_t height_, uint32_t layers_, std::span<VkImageView> attachments_, RenderPass& rRenderpass_)
{
    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass = rRenderpass_.get();
    frameBufferCreateInfo.attachmentCount = attachments_.size();
    frameBufferCreateInfo.pAttachments = attachments_.data();
    frameBufferCreateInfo.width = width_; // EngineGlobals::renderer.swapchain.extent.width
    frameBufferCreateInfo.height = height_; // EngineGlobals::renderer.swapchain.extent.height
    frameBufferCreateInfo.layers = layers_;
    VkResult result = vkCreateFramebuffer(getMainDevice().logicalDevice, &frameBufferCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to createa a FrameBuffer!");
    }
}

void Framebuffer::destroy()
{
    if (this->vkHandle != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(getMainDevice().logicalDevice, this->vkHandle, nullptr);
        this->vkHandle = VK_NULL_HANDLE;
    }
}


VkFramebuffer Framebuffer::get() const
{
    return this->vkHandle;
}
}
