#include "Framebuffer.h"
#include "Globals.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>


void Framebuffer::create(uint32_t width_, uint32_t height_, uint32_t layers_, RenderPass& rRenderpass_)
{
    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass = rRenderpass_.get();
    frameBufferCreateInfo.attachmentCount = attachments.size();
    frameBufferCreateInfo.pAttachments = attachments.data();
    frameBufferCreateInfo.width = width_; // EngineGlobals::renderer.swapchain.extent.width
    frameBufferCreateInfo.height = height_; // EngineGlobals::renderer.swapchain.extent.height
    frameBufferCreateInfo.layers = layers_;
    VkResult result = vkCreateFramebuffer(Demo::renderer.mainDevice.logicalDevice, &frameBufferCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to createa a FrameBuffer!");
    }
}

void Framebuffer::destroy()
{
    vkDestroyFramebuffer(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}


VkFramebuffer Framebuffer::get() const
{
    return this->vkHandle;
}

Framebuffer::~Framebuffer()
{
    this->destroy(); 
}
