#include "app.h"

void App::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicQueueIndex;
    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    CHECK_VKRESULT(result, "failed to create command pool!");
}

std::vector<VkCommandBuffer> App::createCommandBuffers(uint32_t size) {
    LOG("createCommandBuffers");
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = size;
    
    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.resize(size);
    vkAllocateCommandBuffers( m_device, &allocInfo, commandBuffers.data());
    return commandBuffers;
}

VkCommandBuffer App::beginSingleTimeCommands() {
    LOG("beginSingleTimeCommands");
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkCommandBuffer commandBuffer = createCommandBuffers()[0];
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void App::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    LOG("endSingleTimeCommands");
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;
    
    vkQueueSubmit  (m_graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicQueue);
    vkFreeCommandBuffers( m_device, m_commandPool, 1, &commandBuffer);
}
