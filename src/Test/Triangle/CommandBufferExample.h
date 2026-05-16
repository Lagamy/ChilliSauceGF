

VkCommandBuffer cmd = ...; // already allocated

VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
};

vkBeginCommandBuffer(cmd, &beginInfo);

// --- Render pass (render target setup) ---
VkClearValue clearColor = { .color = {{0.1f, 0.1f, 0.1f, 1.0f}} };

VkRenderPassBeginInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = renderPass,          // your VkRenderPass
    .framebuffer = framebuffer,        // target framebuffer
    .renderArea = {
        .offset = {0, 0},
        .extent = swapchainExtent
    },
    .clearValueCount = 1,
    .pClearValues = &clearColor
};

vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

// --- Bind pipeline ---
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

// --- Bind vertex buffer ---
VkBuffer vertexBuffers[] = { vertexBuffer };
VkDeviceSize offsets[] = { 0 };
vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

// --- Optional: viewport & scissor (if dynamic) ---
VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float)swapchainExtent.width,
    .height = (float)swapchainExtent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
};
vkCmdSetViewport(cmd, 0, 1, &viewport);

VkRect2D scissor = {
    .offset = {0, 0},
    .extent = swapchainExtent
};
vkCmdSetScissor(cmd, 0, 1, &scissor);

// --- Draw ---
vkCmdDraw(cmd, vertexCount, 1, 0, 0);

// --- End render pass ---
vkCmdEndRenderPass(cmd);

vkEndCommandBuffer(cmd);
