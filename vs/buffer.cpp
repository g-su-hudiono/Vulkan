//  Copyright © 2021 Subph. All rights reserved.
//

#include "buffer.h"

#include "helper.h"

Buffer::~Buffer() {}
Buffer::Buffer(VkDevice device, VkPhysicalDevice physicalDevice) :
    m_device(device),
    m_physicalDevice(physicalDevice),
    m_bufferInfo(GetDefaultBufferCreateInfo()) {
}

void Buffer::cleanup() {
    vkDestroyBuffer   (m_device, m_buffer      , nullptr);
    vkFreeMemory      (m_device, m_bufferMemory, nullptr);
}

void Buffer::setup(VkDeviceSize size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo bufferInfo = m_bufferInfo;
    
    bufferInfo.size  = size;
    bufferInfo.usage = usage;
    
    m_bufferInfo = bufferInfo;
}

void Buffer::create() {
    createBuffer();
    allocateBufferMemory();
}

void Buffer::createBuffer() {
    LOG("Buffer::createBuffer");
    VkResult result = vkCreateBuffer(m_device, &m_bufferInfo, nullptr, &m_buffer);
    CHECK_VKRESULT(result, "failed to buffer!");
}

void Buffer::allocateBufferMemory() {
    LOG("Buffer::allocateBufferMemory");
    VkDevice         device         = m_device;
    VkPhysicalDevice physicalDevice = m_physicalDevice;
    VkBuffer         buffer         = m_buffer;
    
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);
    
    int32_t memoryTypeIndex;
    memoryTypeIndex = FindMemoryTypeIndex(physicalDevice,
                                          memoryRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VkMemoryAllocateFlagsInfo flagInfo{};
    flagInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memoryRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    allocInfo.pNext           = &flagInfo;
    
    VkDeviceMemory bufferMemory;
    VkResult       result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    CHECK_VKRESULT(result, "failed to allocate buffer memory!");
    
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    
    { m_bufferMemory = bufferMemory; }
}

void* Buffer::fillBuffer(const void* address, VkDeviceSize size, int32_t shift) {
    void* ptr = mapMemory(size);
    ptr = static_cast<char*>(ptr) + shift;
    memcpy(ptr, address, size);
    unmapMemory();
    return ptr;
}

void* Buffer::fillBufferFull(const void* address) {
    return fillBuffer(address, static_cast<size_t>(m_bufferInfo.size));
}

void* Buffer::mapMemory(VkDeviceSize size) {
    void* ptr;
    vkMapMemory(m_device, m_bufferMemory, 0, size, 0, &ptr);
    return ptr;
}

void Buffer::unmapMemory() {
    vkUnmapMemory(m_device, m_bufferMemory);
}

VkBuffer       Buffer::getBuffer      () { return m_buffer;       }
VkDeviceMemory Buffer::getBufferMemory() { return m_bufferMemory; }
VkDeviceSize   Buffer::getBufferSize  () { return m_bufferInfo.size; }
VkDescriptorBufferInfo Buffer::getBufferInfo() {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_buffer;
    bufferInfo.range  = m_bufferInfo.size;
    bufferInfo.offset = 0;
    return bufferInfo;
}



VkBufferCreateInfo Buffer::GetDefaultBufferCreateInfo() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return bufferInfo;
}
