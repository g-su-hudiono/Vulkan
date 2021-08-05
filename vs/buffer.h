//  Copyright © 2021 Subph. All rights reserved.
//

#pragma once

#include "common.h"

class Buffer {
    
public:
    ~Buffer();
    Buffer(VkDevice device, VkPhysicalDevice physicalDevice);

    void cleanup();
    
    unsigned char* m_desc;
    
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    
    VkBuffer         m_buffer         = VK_NULL_HANDLE;
    VkDeviceMemory   m_bufferMemory   = VK_NULL_HANDLE;
    
    VkBufferCreateInfo m_bufferInfo{};
    
    VkBuffer       getBuffer();
    VkDeviceSize   getBufferSize();
    VkDeviceMemory getBufferMemory();
    VkDescriptorBufferInfo getBufferInfo();
    
    void setup (VkDeviceSize size, VkBufferUsageFlags usage);
    void create();
    
    void createBuffer();
    void allocateBufferMemory();
        
    void* fillBuffer    (const void* address, VkDeviceSize size, int32_t shift = 0);
    void* fillBufferFull(const void* address);
    
    void* mapMemory(VkDeviceSize size);
    void  unmapMemory();
    
    
private:
    
    VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
    VkDeviceMemory allocateBufferMemory(VkBuffer& buffer, VkDeviceSize size, int32_t memoryTypeIndex);
    
    static VkBufferCreateInfo GetDefaultBufferCreateInfo();
};
